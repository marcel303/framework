/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "internal.h"
#include "StringEx.h"
#include <algorithm>
#include <math.h>

#if WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <Windows.h> // MAX_PATH
#endif

static AnimCacheElem s_dummyAnimCacheElem;

Sprite::Sprite(const char * filename, float pivotX, float pivotY, const char * spritesheet, bool autoUpdate, bool hasSpriteSheet)
	: m_autoUpdate(autoUpdate)
{
	// drawing
	this->pivotX = pivotX;
	this->pivotY = pivotY;
	x = 0.f;
	y = 0.f;
	angle = 0.f;
	separateScale = false;
	scale = 1.f;
	scaleX = 1.f;
	scaleY = 1.f;
	flipX = false;
	flipY = false;
	pixelpos = true;
	filter = FILTER_POINT;
	
	// animation
	const char * sheetFilename = 0;
#if WINDOWS
	char sheetFilenameBuffer[MAX_PATH];
#else
	char sheetFilenameBuffer[PATH_MAX];
#endif
	if (hasSpriteSheet)
	{
		if (spritesheet)
		{
			sheetFilename = spritesheet;
		}
		else
		{
			strcpy_s(sheetFilenameBuffer, sizeof(sheetFilenameBuffer), filename);
			int dot = -1;
			for (int i = 0; filename[i] != 0; ++i)
				if (filename[i] == '.')
					dot = i;
			if (dot != -1 && dot + 4 < sizeof(sheetFilenameBuffer))
			{
				sheetFilenameBuffer[dot + 1] = 't';
				sheetFilenameBuffer[dot + 2] = 'x';
				sheetFilenameBuffer[dot + 3] = 't';
				sheetFilenameBuffer[dot + 4] = 0;
				sheetFilename = sheetFilenameBuffer;
			}
		}
	}
	
	if (sheetFilename)
		m_anim = &g_animCache.findOrCreate(sheetFilename);
	else
		m_anim = &s_dummyAnimCacheElem;
	m_animVersion = m_anim->getVersion();
	m_animSegment = 0;
	animIsActive = false;
	animIsPaused = false;
	m_isAnimStarted = false;
	m_animFramef = 0.f;
	m_animFrame = 0;
	animSpeed = 1.f;
	animActionHandler = 0;
	animActionHandlerObj = 0;

	if (m_anim->m_hasSheet)
	{
		this->pivotX = (float)m_anim->m_pivot[0];
		this->pivotY = (float)m_anim->m_pivot[1];
		this->scale = (float)m_anim->m_scale;
	}
	
	// texture
	m_texture = &g_textureCache.findOrCreate(filename, m_anim->m_gridSize[0], m_anim->m_gridSize[1], false);
	
	m_prev = 0;
	m_next = 0;
	if (m_autoUpdate)
		framework.registerSprite(this);
}

Sprite::~Sprite()
{
	if (m_autoUpdate)
		framework.unregisterSprite(this);
}

void Sprite::reload()
{
	m_texture->reload();
}

void Sprite::update(float dt)
{
	updateAnimation(dt);
}

void Sprite::draw()
{
	drawEx(x, y, angle, separateScale ? scaleX : scale, separateScale ? scaleY : scale, pixelpos, filter);
}

void Sprite::drawEx(float x, float y, float angle, float scaleX, float scaleY, bool pixelpos, TEXTURE_FILTER filter)
{
	if (scaleY == FLT_MAX)
		scaleY = scaleX;

	if (m_texture->textures)
	{
		gxPushMatrix();
		{
			if (pixelpos)
			{
				x = floorf(x / framework.minification) * framework.minification;
				y = floorf(y / framework.minification) * framework.minification;
			}
			
			gxTranslatef(x, y, 0.f);
			
			if (angle != 0.f)
				gxRotatef(angle, 0.f, 0.f, 1.f);
			if (scaleX != 1.f || scaleY != 1.f)
				gxScalef(scaleX, scaleY, 1.f);
			if (flipX || flipY)
				gxScalef(flipX ? -1.f : +1.f, flipY ? -1.f : +1.f, 1.f);
			if (pivotX != 0.f || pivotY != 0.f)
				gxTranslatef(-pivotX, -pivotY, 0.f);
			
			int cellIndex;
			
			if (m_isAnimStarted && m_animSegment)
			{
				AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
				
				cellIndex = getAnimFrame() + anim->firstCell;
			}
			else
			{
				cellIndex = 0;
			}
			
			fassert(cellIndex >= 0 && cellIndex < (m_anim->m_gridSize[0] * m_anim->m_gridSize[1]));
			
			gxSetTexture(m_texture->textures[cellIndex].id);
			gxSetTextureSampler(
				filter == FILTER_POINT ? GX_SAMPLE_NEAREST :
				filter == FILTER_LINEAR ? GX_SAMPLE_LINEAR :
				filter == FILTER_MIPMAP ? GX_SAMPLE_MIPMAP : GX_SAMPLE_NEAREST,
				true);
			
			const float rsx = float(m_texture->sx / m_anim->m_gridSize[0]);
			const float rsy = float(m_texture->sy / m_anim->m_gridSize[1]);
			
			gxBegin(GX_QUADS);
			{
				gxTexCoord2f(0.f, 0.f); gxVertex2f(0.f, 0.f);
				gxTexCoord2f(1.f, 0.f); gxVertex2f(rsx, 0.f);
				gxTexCoord2f(1.f, 1.f); gxVertex2f(rsx, rsy);
				gxTexCoord2f(0.f, 1.f); gxVertex2f(0.f, rsy);
			}
			gxEnd();
		}
		gxPopMatrix();

		gxSetTexture(0);
	}
}

void Sprite::startAnim(const char * name, int frame)
{
	animIsPaused = false;
	m_animSegmentName = name;
	m_isAnimStarted = true;
	m_animFramef = (float)frame;
	m_animFrame = frame;

	m_animVersion = -1;
	updateAnimationSegment();
	
	if (animIsActive)
	{
		processAnimationTriggersForFrame(m_animFrame, AnimCacheElem::AnimTrigger::OnEnter);
	}
}

void Sprite::stopAnim()
{
	animIsActive = false;
	m_isAnimStarted = false;
	m_animSegment = 0;
}

const std::string & Sprite::getAnim() const
{
	return m_animSegmentName;
}

void Sprite::setAnimFrame(int frame)
{
	fassert(frame >= 0);
	
	if (m_animSegment)
	{
		const int frame1 = m_animFrame;
		{
			m_animFrame = calculateLoopedFrameIndex(frame);
		}
		const int frame2 = m_animFrame;
		
		m_animFramef = (float)m_animFrame;

		processAnimationFrameChange(frame1, frame2);
	}
}

int Sprite::getAnimFrame() const
{
	return m_animFrame;
}

std::vector<std::string> Sprite::getAnimList() const
{
	std::vector<std::string> result;

	if (m_anim)
	{
		for (auto i = m_anim->m_animMap.begin(); i != m_anim->m_animMap.end(); ++i)
			result.push_back(i->first);
	}

	return result;
}

void Sprite::updateAnimationSegment()
{
	if (m_isAnimStarted && m_animVersion != m_anim->getVersion() && !m_animSegmentName.empty())
	{
		m_animVersion = m_anim->getVersion();
		
		if (m_anim->m_animMap.count(m_animSegmentName) != 0)
			m_animSegment = &m_anim->m_animMap[m_animSegmentName];
		else
			m_animSegment = 0;
		
		if (!m_animSegment)
		{
			logInfo("unable to find animation: %s", m_animSegmentName.c_str());
			animIsActive = false;
			m_animFramef = 0.f;
			m_animFrame = 0;
		}
		else
		{
			AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
			
			this->pivotX = (float)anim->pivot[0];
			this->pivotY = (float)anim->pivot[1];
			
			animIsActive = true;
		}
		
		// recache texture, since the animation grid size may have changed
		m_texture = &g_textureCache.findOrCreate(m_texture->name.c_str(), m_anim->m_gridSize[0], m_anim->m_gridSize[1], false);
	}
}

void Sprite::updateAnimation(float timeStep)
{
	if (m_isAnimStarted && m_animSegment && !animIsPaused)
	{
		AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
		const int frame1 = m_animFrame;
		{
			const float step = animSpeed * anim->frameRate * timeStep;
			
			m_animFramef += step;
			
			if (!anim->loop)
				m_animFrame = std::min<int>((int)m_animFramef, anim->numFrames - 1);
			else
				m_animFrame = (int)m_animFramef;
		}
		const int frame2 = m_animFrame;
		
		for (int frame = frame1; frame < frame2; frame++)
		{
			const int oldFrame = calculateLoopedFrameIndex(frame + 0);
			const int newFrame = calculateLoopedFrameIndex(frame + 1);
			processAnimationFrameChange(oldFrame, newFrame);
		}
		
		if (anim->loop)
		{
			while (m_animFramef >= anim->numFrames)
				m_animFramef -= anim->numFrames - anim->loopStart;
			m_animFrame = (int)m_animFramef;
		}
		else
		{
			if (m_animFramef >= anim->numFrames)
				animIsActive = false;
		}
		
		//if (m_animSegmentName == "default")
		//	logInfo("%d (%d)", m_animFrame, anim->numFrames);
	}
}

void Sprite::processAnimationFrameChange(int frame1, int frame2)
{
	fassert(animIsActive);
	
	if (frame1 != frame2)
	{
		// process frame triggers
		processAnimationTriggersForFrame(frame1, AnimCacheElem::AnimTrigger::OnLeave);
		processAnimationTriggersForFrame(frame2, AnimCacheElem::AnimTrigger::OnEnter);
	}
}

void Sprite::processAnimationTriggersForFrame(int frame, int event)
{
	fassert(animIsActive);
	
	AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
	for (size_t i = 0; i < anim->frameTriggers[frame].size(); ++i)
	{
		const AnimCacheElem::AnimTrigger & trigger = anim->frameTriggers[frame][i];
		
		if (trigger.event == event)
		{
			//logInfo("event == this->event");
			
			Dictionary args = trigger.args;
			args.setPtr("obj", animActionHandlerObj);
			args.setInt("x", args.getInt("x", 0) + (int)this->x);
			args.setInt("y", args.getInt("y", 0) + (int)this->y);
			
			if (animActionHandler)
				animActionHandler(trigger.action, args);
			else
				framework.processActions(trigger.action, args);
		}
	}
}

int Sprite::calculateLoopedFrameIndex(int frame) const
{
	AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);

	if (!anim->loop)
		frame = std::max<int>(0, std::min<int>(anim->numFrames - 1, frame));
	else
	{
		while (frame < 0)
			frame += anim->numFrames;
		while (frame >= anim->numFrames)
			frame -= anim->numFrames - anim->loopStart;
	}

	return frame;
}

int Sprite::getWidth() const
{
	return m_texture->sx / m_anim->m_gridSize[0];
}

int Sprite::getHeight() const
{
	return m_texture->sy / m_anim->m_gridSize[1];
}

GxTextureId Sprite::getTexture() const
{
	if (m_texture->textures)
		return m_texture->textures[0].id;
	else
		return 0;
}
