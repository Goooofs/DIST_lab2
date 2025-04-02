#ifndef IBRANCHSTRAT_H
#define IBRANCHSTRAT_H

class BoolEquation;

class IBranchStrat
{
public:
    virtual ~IBranchStrat() = default;

    virtual int chooseColumn(BoolEquation& equation) = 0; 
};

#endif
