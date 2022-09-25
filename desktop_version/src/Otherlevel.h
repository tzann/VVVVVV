#ifndef OTHERLEVEL_H
#define OTHERLEVEL_H

class otherlevelclass
{
public:
    const short* loadlevel(int rx, int ry, int rxoff, int ryoff);

    const char* roomname;
    const char* hiddenname;

    int roomtileset;
};

#endif /* OTHERLEVEL_H */
