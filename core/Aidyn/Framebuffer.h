#pragma once
#include "../ew/external/glad.h"

namespace aidyn {
	struct Framebuffer {
		unsigned int fbo;
		unsigned int colorBuffer[8];
		unsigned int depthBuffer;
		unsigned int width;
		unsigned int height;
	};

	Framebuffer createFramebufferDepth(unsigned int width, unsigned int height, int colorFormat)
	{
		Framebuffer frameBuffer;

		//Create fbo
		glCreateFramebuffers(1, &frameBuffer.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);

		//create and bind colorbuffer
		glGenTextures(1, &frameBuffer.colorBuffer[0]);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.colorBuffer[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frameBuffer.colorBuffer[0], 0);




		//create and bind depth buffer
		glGenTextures(1, &frameBuffer.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frameBuffer.depthBuffer, 0);



		return frameBuffer;

	}

	Framebuffer createFramebuffer(unsigned int width, unsigned int height, int colorFormat)
	{
		Framebuffer frameBuffer;

		//Create fbo
		glCreateFramebuffers(1, &frameBuffer.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);

		//create and bind colorbuffer
		glGenTextures(1, &frameBuffer.colorBuffer[0]);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.colorBuffer[0]);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frameBuffer.colorBuffer[0], 0);



		//create and bind renderbuffer
		glGenRenderbuffers(1, &frameBuffer.depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer.depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frameBuffer.depthBuffer);

		return frameBuffer;

	}

	Framebuffer createShadowBuffer(unsigned int width, unsigned int height)
	{
		Framebuffer frameBuffer;

		//Create fbo
		glCreateFramebuffers(1, &frameBuffer.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);

		//create and bind depth buffer
		glGenTextures(1, &frameBuffer.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frameBuffer.depthBuffer, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		frameBuffer.width = width;
		frameBuffer.height = height;

		return frameBuffer;

	}



	Framebuffer createGBuffer(unsigned int width, unsigned int height) {
		Framebuffer framebuffer;
		framebuffer.width = width;
		framebuffer.height = height;

		glCreateFramebuffers(1, &framebuffer.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

		int formats[3] = {
			GL_RGB32F, //0 = World Position 
			GL_RGB16F, //1 = World Normal
			GL_RGB16F  //2 = Albedo
		};
 	//Create 3 color textures
		for (size_t i = 0; i < 3; i++)
		{
			glGenTextures(1, &framebuffer.colorBuffer[i]);
			glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffer[i]);
			glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);



			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, framebuffer.colorBuffer[i], 0);
		}
		//Explicitly tell OpenGL which color attachments we will draw to
		const GLenum drawBuffers[3] = {
				GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
		};
		glDrawBuffers(3, drawBuffers);
		//TODO: Add texture2D depth buffer
		glGenTextures(1, &framebuffer.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, framebuffer.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, width, height);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebuffer.depthBuffer, 0);




		//TODO: Check for completeness

		GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("Not complete");
		}





		//Clean up global state
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return framebuffer;
	}


}