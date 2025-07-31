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
    BasicTexture(const std::string &file);

    ~BasicTexture();

    void Generate(unsigned int width, unsigned int height, unsigned char* data);
    void Bind() const;


    void LoadFromFile(const char * file);

};


#endif //MINECRAFT_BASICTEXTURE_H
