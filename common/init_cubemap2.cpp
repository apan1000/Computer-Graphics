void init_cubemap2(const char * baseFileName, int size){
	//TODO: implement cubemap initialization
	glActiveTexture(GL_TEXTURE0 + 3);
	glGenTextures(1, &cubeTexID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexID);
	const char * suffixes[] = { "posx", "negx", "posy", "negy", "posz", "negz" };
	GLuint targets[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (int i = 0; i < 6; i++){
		GLint w, h;
		std::string texName = std::string(baseFileName) + "_" + suffixes[i] + ".bmp";
		unsigned char* data = loadBMP_cube(texName.c_str(), &w, &h);
		glTexImage2D(targets[i], 0, GL_RGB, w, h, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	}
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}