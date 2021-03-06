/***************************************************************************
 *
 * Copyright 2010,2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
 * Copyright (C) 2018 Advanced Driver Information Technology Joint Venture GmbH
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#include "Street.h"
#include "ShaderBase.h"

#include <string.h>

#include <iostream>
using std::cout;
using std::endl;

#include <GLES2/gl2.h>


Street::Street(vec3f position, vec3f size, vec4f color, ShaderBase* shader)
: m_position(position)
, m_size(size)
, m_color(color)
, m_shader(shader)
{
    m_index[0] = vec3u(0, 3, 2);
    m_index[1] = vec3u(2, 1, 0);

    //                             y  z
    //     3-------------2         | /
    //    /             /          |/
    //   /             /           ------x
    //  0-------------1

    m_vertex[0].x = m_position.x;
    m_vertex[0].y = m_position.y;
    m_vertex[0].z = m_position.z;

    m_vertex[1].x = m_position.x + m_size.x;
    m_vertex[1].y = m_position.y;
    m_vertex[1].z = m_position.z;

    m_vertex[2].x = m_position.x + m_size.x;
    m_vertex[2].y = m_position.y;
    m_vertex[2].z = m_position.z + m_size.z;

    m_vertex[3].x = m_position.x;
    m_vertex[3].y = m_position.y;
    m_vertex[3].z = m_position.z + m_size.z;
}

Street::Street(vec3f position, vec3f size, vec4f color, ShaderTexture* shader, TextureLoader* streetTexture, float numRepeats)
: Street(position, size, color, shader)
{
    withTexture = true;
    texture = streetTexture;
    float xCoords, yCoords;

    float z_to_x = m_size.z / m_size.x;
    float x_to_z = m_size.x / m_size.z;
    if(z_to_x > x_to_z){
        yCoords = numRepeats;
        xCoords = numRepeats * z_to_x;
    } else {
        yCoords = numRepeats * z_to_x;
        xCoords = numRepeats;
    }

    m_texCoords[0].x = 0.0;
    m_texCoords[0].y = 0.0;

    m_texCoords[1].x = xCoords;
    m_texCoords[1].y = 0.0;

    m_texCoords[2].x = xCoords;
    m_texCoords[2].y = yCoords;

    m_texCoords[3].x = 0.0;
    m_texCoords[3].y = yCoords;
}

void Street::render()
{
    if(withTexture) {
        GLuint textureID = texture->getId();
        ((ShaderTexture *)m_shader)->use(&m_position, textureID);
        ((ShaderTexture *)m_shader)->setTexCoords(m_texCoords);
    }else{
        m_shader->use(&m_position, &m_color);
    }

    // draw
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, m_vertex);
    glDrawElements(GL_TRIANGLES, 3 * sizeof(m_index)/sizeof(m_index[0]), GL_UNSIGNED_SHORT, m_index);
}

void Street::update(int currentTimeInMs, int lastFrameTime)
{
    (void)currentTimeInMs; //prevent warning
    m_position.z += 0.0005 * lastFrameTime;

    if (m_position.z > 3.0)
    {
        m_position.z -= 2.0;
    }
}
