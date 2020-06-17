#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

//������ѡ����Shader��Bufferȥ����ĳ�������Σ�OpenGL��һ��״̬��

//�������� Vertex Attribute �� shader
/*
Vertex Attribute:

opengl pipeline �Ĺ�����ʽ���ṩͼ�����͸����ݣ�Ȼ��洢��GPU�ϵ��ڴ���ڴ��������������Ҫ���Ƶ��������ݣ�

Ȼ������ʹ��shader����GPU��ִ�е�һ�ֳ��򣩶�ȡ�ⲿ�����ݣ�Ȼ������Ļ����ʾ����

�д����Ե��ǣ�����ʵ���ϻ���ͼ��ʹ�õ���һ��Vertex buffer ���洢��GPU�ϵ�һ�����ڴ�� buffer ��

��shader ʵ���Ͽ�ʼ��ȡ Vertex buffer ʱ������Ҫ֪�� buffer �Ĳ��֣� buffer ������ʲô��

������ǲ�˵�������ֻ�Ǻ�c++����������ûʲô����



glVertexAttribPointer() ������

stride: the amount of bytes between each vertex 12 for coordinate(index1), 8 for texture(index2), 12 for normal(index3)(bytes) so the stride is 32 bytes

pointer: ָ�����Ե�ָ�� coordinate offset = 0 ,texture offset = 12, normal offset = 20

*/

/*
��õ����� shader ��

vertex shader OR fragment(pixel) shader

 data(CPU) -> GPU -> draw call -> shader

 Draw Call����CPU����ͼ�α�̽ӿڣ�����DirectX��OpenGL��������GPU������Ⱦ�Ĳ�����

 vertex shader: ���� OpenGL ����Ҫ vertex ��������Ļ�ռ�ĺδ�

 fragment(pixel) shader : դ�񻯣�ÿ��С������ʲô��ɫ

 �ǵ� enable shader

*/

/*

uniform ʵ������һ�ִ� CPU ��ȡ���ݵ� shader ���һ�ַ��� 

*/

#define ASSERT(x) if(!(x)) __debugbreak();
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x/* �Ѻ����������ַ������� */, __FILE__, __LINE__))

static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGL Error] (" << error << " )" << function << " " << file << ":" << line << std::endl;
		return false;
	}
	return true;
}

struct ShaderProgramSources
{
	std::string VertexSource;
	std::string FragmentSource;
};

static ShaderProgramSources ParseShader(const std::string& filepath)
{
	std::ifstream stream(filepath);

	enum class ShaderType
	{
		NONE = -1, VERTEX = 0, FRAGMENT = 1
	};

	std::string line;
	std::stringstream ss[2];
	ShaderType type = ShaderType::NONE;
	while (getline(stream, line))
	{
		if (line.find("#shader") != std::string::npos)
		{
			if (line.find("vertex") != std::string::npos)
				type = ShaderType::VERTEX;
			else if (line.find("fragment") != std::string::npos)
				type = ShaderType::FRAGMENT;
		}
		else
		{
			ss[(int)type] << line << '\n';
		}
	}

	return { ss[0].str(), ss[1].str() };
}

static unsigned int CompiledShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();// &source[0]; ��ȷ source ָ�����һ����Ч�ĵ�ַ
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	//Error Handling
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));//allocate on the stack
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compiled "<< 
			(type==GL_VERTEX_SHADER?"Vertex":"Fragment") <<" Shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);

		return 0;
	}

	return id;
}

// get a buffer return an ID 
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompiledShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompiledShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;


	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

		if (glewInit() != GLEW_OK)
		std::cout << "Error!" << std::endl;
	//First you need to create a valid OpenGL rendering context and call glewInit() to initialize the extension entry points. 

	std::cout << glGetString(GL_VERSION) << std::endl;

	float positions[] = {//��ʱ�����

		-0.5f, -0.5f, //0
		 0.5f, -0.5f, //1
		 0.5f,  0.5f, //2

		  //0.5f, 0.5f,
		 -0.5f, 0.5f, //3
		 //-0.5f,-0.5f
	};//vertex shader ���������

	unsigned int indices[] = {
		0,1,2,
		2,3,0
	};

	unsigned int buffer;
	GLCall(glGenBuffers(1, &buffer));//create an ID :buffer

	//���� PS һ����ָ�����ϻ���
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));

	//specify the buffer
	GLCall(glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), positions, GL_STATIC_DRAW));

	unsigned int vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	//!!!!! REMEMBER!!!!!!!
	GLCall(glEnableVertexAttribArray(0));

	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0));//2: component count

	unsigned int ibo; // index buffer object
	GLCall(glGenBuffers(1, &ibo));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW)); // indices ��������Ԫ��,������ unsigned ��
		
																		//stride ʵ������һ���㣨����˵һ��2ά��������������ɣ����������������� float ���ͳ��ȣ�
	////������һ����ϵ����λ�ȡ��������ݣ� L146in vec4 position;
	//std::string vertexShader =
	//	"#version 330 core\n"
	//	"\n"
	//	"layout(location = 0) in vec4 position;"// OpenGL��gl_Position��Ҫ��һ��4ά��vector
	//	"\n"
	//	"void mian()\n"
	//	"{\n"glEnableVertexAttribArra
	//	"	gl_Position = position;\n"
	//	"}\n";

	//std::string fragmentShader =
	//	"#version 330 core\n"
	//	"\n"
	//	"layout(location = 0) out vec4 color;"// OpenGL��gl_Position��Ҫ��һ��4ά��vector
	//	"\n"
	//	"void mian()\n"
	//	"{\n"
	//	"	color = vec4(0.0, 1.0, 0.0, 1.0);\n"
	//	"}\n";

	

	ShaderProgramSources source = ParseShader("res/shaders/Basic.shader");
	std::cout << "Vertex********" << std::endl;
	std::cout << source.VertexSource << std::endl;
	std::cout << "Fragment*******" << std::endl;
	std::cout << source.FragmentSource << std::endl;
	unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);//����������
	glUseProgram(shader);//Ԥ����Կ��йأ�
	/*

	��ʾ������ɫ��������ΪglUseProgram(shader) ���ش���GL_INVALID_OPERATION
	����һ��YouTube����Ҳ������������⣬����mac��win���У������Ʋ������intel�Կ���Ӣΰ���OpenGL implement ��һ����

	*/

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		/*glBegin(GL_TRIANGLES);
		glVertex2d(-0.5f, -0.5f);
		glVertex2d(0.0f, 0.5f);
		glVertex2d(0.5f, -0.5f);
		glEnd();*/

		//glDrawArrays(GL_TRIANGLES, 0, 6);//��buffer���*�鿪ʼdraw��һ��*���

		//GLClearError();
		//Draw Call
		GLCall(glDrawElements(GL_TRIANGLES, 6/* ����indices����6��index */, GL_UNSIGNED_INT, nullptr));//��Ϊǰ��glBindBuffer�Ѿ����������Կ�ָ��
		// GL_UNSIGNED_INT �����д����GL_INT���ᵼ�º���
		//GLCheckError();
		//ASSERT(GLLogCall());



		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glDeleteProgram(shader);

	glfwTerminate();
	return 0;
}