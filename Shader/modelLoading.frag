#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

/*
    Indica si el material actual tiene
    una textura difusa.
*/
uniform bool hasTexture;

/*
    Color Kd del material cuando
    no existe textura.
*/
uniform vec3 materialDiffuse;

void main()
{
    if (hasTexture)
    {
        vec4 texColor = texture(
            texture_diffuse1,
            TexCoords
        );

        /*
            Evita dibujar texels completamente
            transparentes si alguna textura
            contiene canal alpha.
        */
        if (texColor.a < 0.05)
            discard;

        FragColor = texColor;
    }
    else
    {
        FragColor = vec4(
            materialDiffuse,
            1.0
        );
    }
}