//
// Created by 郭珂桢 on 2024/5/20.
//

#ifndef MINECRAFT_BASICTEXTURE_H
#define MINECRAFT_BASICTEXTURE_H



#include "../Util/NonCopyable.h"

#include <string>

class BasicTexture
{
public:
    unsigned int ID;
    unsigned int Width, Height;

    BasicTexture() {}
    explicit BasicTexture(const std::string &file, int atlasTileSize = 0);

    ~BasicTexture();

    void Generate(unsigned int width, unsigned int height, unsigned char* data,
                  int atlasTileSize = 0);
    void Bind() const;


    void LoadFromFile(const char * file, int atlasTileSize = 0);

};


#endif //MINECRAFT_BASICTEXTURE_H
