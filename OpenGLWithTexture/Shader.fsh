precision mediump float;

uniform sampler2D sTexture_y;
uniform sampler2D sTexture_u;
uniform sampler2D sTexture_v;

varying vec2 vTextureCoord;

//公式转码
void getRgbByYuv(in float y, in float u, in float v, inout float r, inout float g, inout float b){	
	
    y = 1.164*(y - 0.0625);
    u = u - 0.5;
    v = v - 0.5;
    
    r = y + 1.596023559570*v;
    g = y - 0.3917694091796875*u - 0.8129730224609375*v;
    b = y + 2.017227172851563*u;
}

void main() {
 	float r,g,b;
 	
 	// 采样出YUV
 	float y = texture2D(sTexture_y, vTextureCoord).r;
    	float u = texture2D(sTexture_u, vTextureCoord).r;
    	float v = texture2D(sTexture_v, vTextureCoord).r;
	// 转码公式
	getRgbByYuv(y, u, v, r, g, b);
	
	// 最终颜色赋值
	gl_FragColor = vec4(r,g,b, 1.0); 
}
 
