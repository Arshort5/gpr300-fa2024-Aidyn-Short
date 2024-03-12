#pragma once
#include "../ew/external/glad.h"

namespace aidyn {
	struct Framebuffer {
		unsigned int fbo;
		unsigned int colorBuffer;
		unsigned int depthBuffer;
	};

	Framebuffer createFramebufferDepth(unsigned int width, unsigned int height, int colorFormat)
	{
		Framebuffer frameBuffer;

		//Create fbo
		glCreateBuffers(1, &frameBuffer.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer.fbo);

		//create and bind colorbuffer
		glGenTextures(1, &frameBuffer.colorBuffer);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.colorBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D, frameBuffer.colorBuffer, 0);

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
		glGenTextures(1, &frameBuffer.colorBuffer);
		glBindTexture(GL_TEXTURE_2D, frameBuffer.colorBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, width, height);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frameBuffer.colorBuffer, 0);



		//create and bind renderbuffer
		glGenRenderbuffers(1, &frameBuffer.depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer.depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frameBuffer.depthBuffer);

		return frameBuffer;

	}
}