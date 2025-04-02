#ifndef MINOCCSTRATEGY_H
#define MINOCCSTRATEGY_H

#include "IBranchStrat.h"
#include "boolequation.h"
#include <vector>
#include <algorithm>

class MinOccStrategy : public IBranchStrat
{
public:
    // Переопределяем метод интерфейса 
    int chooseColumn(BoolEquation& equation) override;
};

#endif
