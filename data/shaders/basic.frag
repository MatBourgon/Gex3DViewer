#version 330 core

out vec4 FragColor;
in vec4 vCol;
in vec2 vUV;
uniform sampler2D uTexture;
uniform int uWireframe;
uniform int uBillboard;

void main()
{
    if (uWireframe == 1) {
        FragColor = vec4(0, 1, 1, 1);
    } else {
        vec4 vertCol = vec4(1,1,1,1);
        vec4 texCol = vec4(1,1,1,1);
        if ((uWireframe & 2) == 2)
        {
            vertCol = 2 * vec4(vCol.rgba);
        }
        if ((uWireframe & 4) == 4)
        {
            texCol = texture2D(uTexture, vUV);
        }
        

        FragColor = texCol * vertCol;

        if (FragColor.a < 0.1) {
            discard;
        }

        //if (uBillboard == 1)
        //{
        //    if (FragColor.r < 0.1 && FragColor.g < 0.1 && FragColor.b < 0.1)
        //    {
        //        discard;
        //    }
        //}
            
    }
    //FragColor = vCol;//vec4(vCol.rgba, 1.0);
}