#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QStack>
#include <string>
#include <cstring>
#include <stack>
#include <ostream>
#include "NodeBoolTree.h"
#include "boolinterval.h"
#include "boolequation.h"
#include "BBV.h"
#include "Allocator.h"
#include "MinOccStrategy.h" // подключаю стратегию
#include "IBranchStrat.h"


int main(int argc, char *argv[])
{
    const size_t kBoolIntervalSize = 64;
    const size_t kBoolEquationSize = 256;
    const size_t kNodeBoolTreeSize = 80;

    Allocator allocBoolInterval(kBoolIntervalSize, 200);
    Allocator allocBoolEquation(kBoolEquationSize, 100);
    Allocator allocNodeBoolTree(kNodeBoolTreeSize, 100);
    Allocator allocCNF(1024, 1);
	Allocator allocStrategy(64, 1);

	QStringList full_file_list;
	QList<QStringList> Elements;
	std::string filepath;
	QStringList inputs;
	filepath = "/home/goof/TSU/Development of information security tools/DIST_lab2/SAT_DPLL/SatExamples/Sat_ex14_3.pla";
	QFile file(QString::fromUtf8(filepath.c_str()));

	BoolInterval** CNF = nullptr;
    int cnfSize = 0;
    BoolInterval* root = nullptr;
    IBranchStrat* strategy = nullptr;
    NodeBoolTree* startNode = nullptr;
	BoolEquation* boolequation = nullptr; 

	//считываем весь файл
	if ((file.exists()) && (file.open(QIODevice::ReadOnly))) {
		while (!file.atEnd()) {
			full_file_list << file.readLine().replace("\r\n", "");
		}

		// int cnfSize = -1;
        int cnfSize = full_file_list.length();
        BoolInterval** CNF = nullptr;
        try{
            size_t requestSize = sizeof(BoolInterval*) * cnfSize;
            // std::cout << requestSize << std::endl;

            if (requestSize > 1024) throw std::runtime_error("CNF array too large for allocator");

            void* memCNF = allocCNF.Allocate(requestSize);
            CNF = static_cast<BoolInterval**>(memCNF);
        } catch (const std::exception& e) {
            std::cerr << "error while allocating CNF: " << e.what() << std::endl;
            return 1;
        }

		int rangInterval = -1; // error

		if (cnfSize) {
			rangInterval = full_file_list[0].toUtf8().trimmed().length();
		}

		for (int i = 0; i < cnfSize; i++) { // Заполняем массив
			QString strv = full_file_list[i];
            
            try {
                if (sizeof(BoolInterval) > kBoolIntervalSize) throw std::runtime_error("BoolInterval size too large for allocator");
                void* mem = allocBoolInterval.Allocate(sizeof(BoolInterval));
                CNF[i] = new (mem) BoolInterval(strv.toUtf8().trimmed().data());
            } catch (const std::exception& e) {
                std::cerr << "error while allocating BoolInterval" << i << ": " << e.what() << std::endl;
                return 1;
            }
		}

		QString rootvec = "";
		QString rootdnc = "";

		//Строим интервал в которм все компоненты принимают значение '-',
		//который представляет собой корень уравнения, пока пустой.
		//В процессе поиска корня, компоненты интервала буду заменены на конкретные значения.

		for (int i = 0; i < rangInterval; i++) {
			rootvec += "0";
			rootdnc += "1";
		}

		QByteArray v = rootvec.toUtf8();

		BBV vec(v.data());
		QByteArray d = rootdnc.toUtf8();
		BBV dnc(d.data());

        try {
            if (sizeof(BoolInterval) > kBoolIntervalSize) throw std::runtime_error("Root BoolInterval size too large for allocator");
            void* memRoot = allocBoolInterval.Allocate(sizeof(BoolInterval));
            root = new (memRoot) BoolInterval(vec, dnc);
        } catch (const std::exception& e) {
            std::cerr << "error while allocating Root BoolInterval: " << e.what() << std::endl;
            return 1;
        }

        try {
            if (sizeof(BoolEquation) > kBoolEquationSize) throw std::runtime_error("BoolEquation size too large for allocator");
            void* mem = allocBoolEquation.Allocate(sizeof(BoolEquation));
			void* memStrategy = allocStrategy.Allocate(sizeof(MinOccStrategy));
            strategy = new (memStrategy) MinOccStrategy(); 
            boolequation = new (mem) BoolEquation(CNF, root, cnfSize, cnfSize, vec, strategy);
        } catch (const std::exception& e) {
            std::cerr << "error while allocating BoolEquation: " << e.what() << std::endl;
            return 1;
        }

		// Алгоритм поиска корня. Работаем всегда с верхушкой стека.
		// Шаг 1. Правила выполняются? Нет - Ветвление Шаг 5. Да - Упрощаем Шаг 2.
		// Шаг 2. Строки закончились? Нет - Шаг1, Да - Корень найден? Да - Успех КОНЕЦ, Нет - Шаг 3.
		// Шаг 3. Кол-во узлов в стеке > 1? Нет - Корня нет КОНЕЦ, Да - Шаг 4.
		// Шаг 4. Текущий узел выталкиваем из стека, попадаем в новый узел. У нового узла lt rt отличны от NULL? Нет - Шаг 1. Да - Шаг 3.
		// Шаг 5. Выбор компоненты ветвления, создание двух новых узлов, добавление их в стек сначала с 1 потом с 0. Шаг 1.

		// Алгоритм CheckRules.
		// Цикл по строкам КНФ.
		// 1. Проверка правила 2. Выполнилось? Да - Корня нет, Нет - Идем дальше.
		// 2. Проверка правила 1. Выполнилось? Да - Упрощаем, Нет - Идем дальше.

		// Создаем стек под узлы булева дерева
		// QStack<NodeBoolTree> BoolTree;

		bool rootIsFinded = false;
		stack<NodeBoolTree *> BoolTree;

        try {
            if (sizeof(NodeBoolTree) > kNodeBoolTreeSize) throw std::runtime_error("NodeBoolTree size too large for allocator");
            void* memNode = allocNodeBoolTree.Allocate(sizeof(NodeBoolTree));
            startNode = new (memNode) NodeBoolTree(boolequation);
        } catch (const std::exception& e) {
            std::cerr << "error while allocating Start NodeBoolTree: " << e.what() << std::endl;
            return 1;
        }

		BoolTree.push(startNode);

		do {
			NodeBoolTree *currentNode(BoolTree.top());

			if (currentNode->lt == nullptr &&
					currentNode->rt == nullptr) { // Если вернулись в обработанный узел
				BoolEquation *currentEquation = currentNode->eq;
				bool flag = true;

				// Цикл для упрощения по правилам.
				while (flag) {
					int a = currentEquation->CheckRules(); // Проверка выполнения правил

					switch (a) {
						case 0: { // Корня нет.
							BoolTree.pop();
							flag = false;
							break;
						}

						case 1: { // Правило выполнилось, корень найден или продолжаем упрощать.
							if (currentEquation->getCount() == 0 ||
									currentEquation->getMask().getWeight() ==
									currentEquation->getMask().getSize()) { // Если кончились строки или столбцы, корень найден.
								flag = false;
								rootIsFinded =
									true; // Полагаем, что корень найден, выполняем проверку корня

								for (int i = 0; i < cnfSize; i++) {

									if (!CNF[i]->isEqualComponent(*currentEquation->getRoot())) {
										rootIsFinded = false;//Корень не найден. Продолжаем искать дальше.
										BoolTree.pop();
										break;
									}
								}
							}

							break;
						}

						case 2: { // Правила не выполнились, ветвление.
							// Ветвление, создание новых узлов.

							int indexBranching = currentEquation->ChooseColForBranching();

                            BoolEquation *Equation0 = nullptr, *Equation1 = nullptr;
                            NodeBoolTree *Node0 = nullptr, *Node1 = nullptr;

                            try {
                                if (sizeof(BoolEquation) > kBoolEquationSize) throw std::runtime_error("Equation size too large for allocator");
                                if (sizeof(NodeBoolTree) > kNodeBoolTreeSize) throw std::runtime_error("Node size too large for allocator");

                                void* memEq0 = allocBoolEquation.Allocate(sizeof(BoolEquation));
                                Equation0 = new (memEq0) BoolEquation(*currentEquation);
                                void* memEq1 = allocBoolEquation.Allocate(sizeof(BoolEquation));
                                Equation1 = new (memEq1) BoolEquation(*currentEquation);

                                Equation0->Simplify(indexBranching, '0');
                                Equation1->Simplify(indexBranching, '1');

                                void* memNode0 = allocNodeBoolTree.Allocate(sizeof(NodeBoolTree));
                                Node0 = new (memNode0) NodeBoolTree(Equation0);
                                void* memNode1 = allocNodeBoolTree.Allocate(sizeof(NodeBoolTree));
                                Node1 = new (memNode1) NodeBoolTree(Equation1);
                            } catch (const std::exception& e) {
                                std::cerr << "error while allocating Branch: " << e.what() << std::endl;
                                return 1;
                            }

							currentNode->lt = Node0;
							currentNode->rt = Node1;

							BoolTree.push(Node1);
							BoolTree.push(Node0);

							flag = false;
							break;
						}
					}
				}
			} else {
				BoolTree.pop();
			}

		} while (BoolTree.size() > 1 && !rootIsFinded);

		if (rootIsFinded) {
			cout << "Root is:\n ";
			BoolInterval *finded_root = BoolTree.top()->eq->getRoot();
			cout << string(*finded_root);
		} else {
			cout << "Root is not exists!";
		}

	} else {
		std::cout << "File does not exists.\n";
	}

	if (startNode) {
        std::stack<NodeBoolTree*> freeStack;
        freeStack.push(startNode);
        while (!freeStack.empty()) {
            NodeBoolTree* node = freeStack.top();
            freeStack.pop();
            if (node->lt) freeStack.push(node->lt);
            if (node->rt) freeStack.push(node->rt);

            if (node->eq) {
                node->eq->~BoolEquation();
                allocBoolEquation.Deallocate(node->eq);
                node->eq = nullptr;
            }
            node->~NodeBoolTree();
            allocNodeBoolTree.Deallocate(node);
        }
    }

    if (CNF) {
        for (int i = 0; i < cnfSize; i++) {
            if (CNF[i]) {
                CNF[i]->~BoolInterval();
                allocBoolInterval.Deallocate(CNF[i]);
                CNF[i] = nullptr;
            }
        }
        allocCNF.Deallocate(CNF);
        CNF = nullptr;
    }

    if (root) {
        root->~BoolInterval();
        allocBoolInterval.Deallocate(root);
        root = nullptr;
    }

    if (strategy) {
    	strategy->~IBranchStrat();
        allocStrategy.Deallocate(strategy);
        strategy = nullptr;
    }

	return 0;
}
