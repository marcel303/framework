#include "Font.h"

class Test
{
public:
	void Test1()
	{
		GLuint texture;
		
		//m_Helper.DrawString(texture);
		
		glDeleteTextures(1, &texture);
	}
	
private:
	FontHelper m_Helper;
};
