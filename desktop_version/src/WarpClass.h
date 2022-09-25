#ifndef WARPCLASS_H
#define WARPCLASS_H

class warpclass
{
public:
    const short* loadlevel(int rx, int ry, int rxoff, int ryoff);
    const char* roomname;
    int rcol;
    bool warpx, warpy;
};

#endif /* WARPCLASS_H */
