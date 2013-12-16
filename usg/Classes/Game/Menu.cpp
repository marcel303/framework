#include "Atlas_ImageInfo.h"
#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Menu.h"
#include "MenuMgr.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "TouchInfo.h"
#include "UsgResources.h"
#include "VectorShape.h"

#include "ParticleGenerator.h"

#define MENU_CURSOR_WRAPAROUND 0
#define MENU_CURSOR_NOSTEEPANGLE 1
#define MENU_CURSOR_NOSTEEPANGLE_MAXX 50
#define MENU_CURSOR_NOSTEEPANGLE_MAXY 80

namespace GameMenu
{
	Menu::Menu()
	{
		m_State = MenuState_Undefined;
		
		m_Focus = 0;
		
		m_TransitionDuration = 0.0f;
		m_TransitionProgress = 0.0f;
	}
	
	Menu::~Menu()
	{
		for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
		{
			delete node->m_Object;
			node->m_Object = 0;
		}

		m_Elements.Clear();
	}
	
	void Menu::Init()
	{
	}
	
	void Menu::SetTransition(TransitionEffect effect, Vec2F vector, float duration)
	{
		m_TransitionEffect = effect;
		m_TransitionVector = vector;
		m_TransitionDuration = duration;
	}
	
	void Menu::Activate()
	{
		State_set(MenuState_ACTIVATE);
	}
	
	void Menu::Deactivate()
	{
		State_set(MenuState_DEACTIVE);
	}
	
	void Menu::Update(float dt)
	{
		m_TransitionProgress += dt;
		
		if (m_TransitionProgress > m_TransitionDuration)
			m_TransitionProgress = m_TransitionDuration;
		
		switch (m_State)
		{
			case MenuState_ACTIVE:
			{
				for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
					node->m_Object->Update(dt);
				
				break;
			}

			case MenuState_DEACTIVE:
			{
				break;
			}

			case MenuState_ACTIVATE:
			{
				// update transition
				
				float progress = 1.0f;
				
				if (m_TransitionDuration != 0.0f)
					progress = m_TransitionProgress / m_TransitionDuration;
				
				for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
					node->m_Object->UpdateTransition(progress);
				
				if (m_TransitionProgress == m_TransitionDuration)
				{
					State_set(MenuState_ACTIVE);
				}
						
				break;
			}

			case MenuState_DEACTIVATE:
			{
				// update transition
				
				float progress = 0.0f;

				if (m_TransitionDuration != 0.0f)
					progress = 1.0f - m_TransitionProgress / m_TransitionDuration;
				
				for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
					node->m_Object->UpdateTransition(progress);
				
				if (m_TransitionProgress == m_TransitionDuration)
				{
					State_set(MenuState_DEACTIVE);
				}

				break;
			}

			case MenuState_Undefined:
				break;
				
#ifndef DEPLOYMENT
			default:
				throw ExceptionVA("unknown menu state: %d", (int)m_State);
#else
			default:
				State_set(MenuState_ACTIVE);
				break;
#endif
		}
	}

	bool Menu::Render()
	{
		switch (m_State)
		{
			case MenuState_DEACTIVE:
				return false;
				
			case MenuState_Undefined:
				return false;

			default:
			{
				for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
				{
					if (!node->m_Object->m_IsVisible)
						continue;
					
					node->m_Object->Render();

#if defined(PSP) && 0
					if (node == m_Focus)
					{
						RectF rect = node->m_Object->HitBox_get();

						RenderRect(rect.m_Position, rect.m_Size, 1.0f, 0.0f, 0.0f, 0.5f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
					}
#endif
				}
				return true;
			}
		}
	}	

	void Menu::HandleFocus()
	{
		MoveToDefault();
	}
	
	bool Menu::HandleTouchBegin(const TouchInfo& touchInfo)
	{
		if (m_State != MenuState_ACTIVE)
			return false;
		
		for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
		{
			IGuiElement* element = node->m_Object;
			
			if (!element->m_IsVisible)
				continue;
			if (!element->m_IsEnabled)
				continue;
			
			if (!element->HitTest(touchInfo.m_LocationView))
				continue;
			
			Focus_set(node);
			
			element->HandleTouchBegin(touchInfo.m_LocationView);
			
			return true;
		}
		
		Focus_set(0);
		
		return false;
	}
	
	bool Menu::HandleTouchMove(const TouchInfo& touchInfo)
	{
		if (m_State != MenuState_ACTIVE)
			return false;
		
		if (m_Focus == 0)
			return false;
		
		Focus_get()->HandleTouchMove(touchInfo.m_LocationView);
		
		return true;
	}
	
	bool Menu::HandleTouchEnd(const TouchInfo& touchInfo)
	{
		if (m_State != MenuState_ACTIVE)
			return false;
		
		if (m_Focus == 0)
			return false;
		
		return true;
	}

	bool Menu::HandleEvent(const Event& e)
	{
		do
		{
			if (m_Focus == 0)
				continue;

			IGuiElement* element = m_Focus->m_Object;

			if (!element->m_IsVisible)
				continue;
			if (!element->m_IsEnabled)
				continue;

			return element->HandleEvent(e);

		} while (false);

		return false;
	}

	void Menu::HandleBack()
	{
	}

	bool Menu::HandlePause()
	{
		return false;
	}

	void Menu::HandleSelect()
	{
		if (m_Focus == 0)
			return;

		IGuiElement* element = m_Focus->m_Object;

		Vec2F location = element->HitBoxCenter_get();

		element->HandleTouchBegin(location);
	}

	void Menu::MoveNext()
	{
		if (m_Focus == 0)
		{
			MoveToDefault();
		}
		else
		{
			Focus_set(FindRelative(+1));
		}
	}

	void Menu::MovePrev()
	{
		if (m_Focus == 0)
		{
			MoveToDefault();
		}
		else
		{
			Focus_set(FindRelative(-1));
		}
	}

	Col::ListNode<IGuiElement*>* Menu::FindRelative(int direction)
	{
		Assert(m_Focus != 0);

		if (direction < 0)
		{
			for (Col::ListNode<IGuiElement*>* node = m_Focus->m_Prev; node != m_Focus; node = node ? node->m_Prev : m_Elements.m_Tail)
			{
				if (node == 0)
					continue;

				if (!node->m_Object->m_IsEnabled)
					continue;
				if (!node->m_Object->m_IsVisible)
					continue;
				if (node->m_Object->m_IsTouchOnly)
					continue;

				return node;
			}
		}

		if (direction > 0)
		{
			for (Col::ListNode<IGuiElement*>* node = m_Focus->m_Next; node != m_Focus; node = node ? node->m_Next : m_Elements.m_Head)
			{
				if (node == 0)
					continue;

				if (!node->m_Object->m_IsEnabled)
					continue;
				if (!node->m_Object->m_IsVisible)
					continue;
				if (node->m_Object->m_IsTouchOnly)
					continue;

				return node;
			}
		}

		return m_Focus;
	}
	
	void Menu::AddElement(IGuiElement* element)
	{
		const GuiElementType type = element->Type_get();

		if (type == GuiElementType_Button ||
			type == GuiElementType_CheckBox ||
			type == GuiElementType_Slider ||
			type == GuiElementType_ListSlider)
		{
			GuiElementBase* base = static_cast<GuiElementBase*>(element);
			if (base->m_TransitionEffect == TransitionEffect_Menu)
				base->SetTransition(m_TransitionEffect, m_TransitionVector);
		}

		m_Elements.AddTail(element);
	}

	IGuiElement* Menu::FindElement(const char* name)
	{
		IGuiElement* element;
		
		if (!TryFindElement(name, &element))
			throw ExceptionVA("element not found: %s", name);

		return element;
	}

	bool Menu::TryFindElement(const char* name, IGuiElement** o_element)
	{
		Assert(o_element != 0);

		for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
		{
			if (!node->m_Object->m_Name)
				continue;
			
			if (!strcmp(node->m_Object->m_Name, name))
			{
				*o_element = node->m_Object;
				return true;
			}
		}

		*o_element = 0;
		return false;
	}

	void Menu::AddButton(const Button& _b)
	{
		Button b = _b;
		
		Button* temp = new Button();
		*temp = b;

		AddElement(temp);
	}

	Col::ListNode<IGuiElement*>* Menu::FindNearestElement(int dx, int dy)
	{
		if (m_Elements.Count_get() == 0)
			return 0;

		if (m_Focus == 0 || m_Elements.Count_get() == 1)
		{
			for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
			{
				if (!node->m_Object->m_IsVisible)
					continue;
				if (!node->m_Object->m_IsEnabled)
					continue;
				if (node->m_Object->m_IsTouchOnly)
					continue;

				return node;
			}

			return m_Elements.m_Head;
		}

		Col::ListNode<IGuiElement*>* min = 0;
		Col::ListNode<IGuiElement*>* max = 0;
		float minDistanceSq = 0.0f;
		float maxDistanceSq = 0.0f;

		for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
		{
			if (node == m_Focus)
				continue;
			if (!node->m_Object->m_IsVisible)
				continue;
			if (!node->m_Object->m_IsEnabled)
				continue;
			if (node->m_Object->m_IsTouchOnly)
				continue;

			Vec2F delta = node->m_Object->HitBoxCenter_get() - m_Focus->m_Object->HitBoxCenter_get();

#if MENU_CURSOR_NOSTEEPANGLE
			float maxAngle = (dy == 0) ? MENU_CURSOR_NOSTEEPANGLE_MAXX : MENU_CURSOR_NOSTEEPANGLE_MAXY;

			Vec2F vec1(dx, dy);
			Vec2F vec2(delta[0], delta[1]);

			vec1.Normalize();
			vec2.Normalize();

			float dot = Calc::Abs(vec1 * vec2);

			if (dot <= cosf(Calc::DegToRad(maxAngle)))
				continue;
#endif

			if (dx != 0)
				delta[0] *= dx;
			else
				delta[0] *= delta[0];
			if (dy != 0)
				delta[1] *= dy;
			else
				delta[1] *= delta[1];

#if MENU_CURSOR_WRAPAROUND
			if (delta[0] < 0.0f)
			{
				delta[0] += VIEW_SX;
				delta[0] *= delta[0];
			}
			if (delta[1] < 0.0f)
			{
				delta[1] += VIEW_SY;
				delta[1] *= delta[1];
			}
#else
			if (dx != 0 && delta[0] <= 0.0f)
				continue;
			if (dy != 0 && delta[1] <= 0.0f)
				continue;
#endif

			const float distanceSq = delta.LengthSq_get();

			//LOG_DBG("delta: %03.2f, %03.2f", delta[0], delta[1]);

			if ((min == 0 || distanceSq < minDistanceSq) && (dx == 0 || (int)delta[0] > 0) && (dy == 0 || (int)delta[1] > 0) && distanceSq >= 1.0f)
			{
				minDistanceSq = distanceSq;
				min = node;
			}

			//if (max == 0 || distance > maxDistance)
			if ((max == 0 || distanceSq < maxDistanceSq) && (dx == 0 || (int)delta[0] < 0) && (dy == 0 || (int)delta[1] < 0) && distanceSq >= 1.0f)
			{
				maxDistanceSq = distanceSq;
				max = node;
			}
		}

		Col::ListNode<IGuiElement*>* result = 0;

		if (min != 0)
		{
			LOG_DBG("result: min", 0);
			result = min;
		}
		else if (max != 0)
		{
			LOG_DBG("result: max", 0);
			result = max;
		}
		else
		{
#if MENU_CURSOR_WRAPAROUND
			result = m_Elements.m_Head;
#endif
		}

		//LOG_DBG("active pos: %f, %f", result->m_Object->HitBoxCenter_get()[0], result->m_Object->HitBoxCenter_get()[1]);

		return result;
	}

	void Menu::MoveToDefault()
	{
		Focus_set(0);
		Focus_set(FindNearestElement(0, 0));
	}

	void Menu::MoveSelection(int dx, int dy)
	{
		Col::ListNode<IGuiElement*> * nearest = FindNearestElement(dx, dy);

		if (nearest != 0)
		{
			Focus_set(nearest);
		}
	}
	
	void Menu::State_set(MenuState state)
	{
		switch (state)
		{
			case MenuState_ACTIVATE:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_MENU_CLICK, SfxFlag_MustFinish);
				m_TransitionProgress = 0.0f;
				HandleFocus();
				break;
				
			case MenuState_DEACTIVATE:
				m_TransitionProgress = 0.0f;
				break;
				
			case MenuState_ACTIVE:
				break;
				
			case MenuState_DEACTIVE:
				break;
				
			case MenuState_Undefined:
				break;
		}
		
		m_State = state;
	}
	
	IGuiElement* Menu::Focus_get()
	{
		if (m_Focus == 0)
			return 0;
		
		return m_Focus->m_Object;
	}

	void Menu::Focus_set(Col::ListNode<IGuiElement*>* focus)
	{
		//if (focus == m_Focus)
		//	return;

		Col::ListNode<IGuiElement*>* oldFocus = m_Focus;

		if (m_Focus)
			m_Focus->m_Object->HandleFocusChanged(false);

		m_Focus = focus;

		if (m_Focus)
			m_Focus->m_Object->HandleFocusChanged(true);

		// fixme: ugly hack..
		g_GameState->m_MenuMgr->HandleFocus(oldFocus ? oldFocus->m_Object : 0, Focus_get());
	}

	void Menu::Translate(int dx, int dy)
	{
		for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
			node->m_Object->Translate(dx, dy);
	}
	
	void ViewName_Render(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		
		ViewNameSnap snap = (ViewNameSnap)button->m_Info;
		
		float sx1 = 310.0f;
		float sx2 = 290.0f;
		float sx3 = 270.0f;
		float sy = 45.0f;
		
		Vec2F p1;
		Vec2F dir;
		
		TextAlignment alignment = TextAlignment_Center;
		
		switch (snap)
		{
			case ViewNameSnap_TopRight:
				p1.Set((float)VIEW_SX, 0.0f);
				dir.Set(-1.0f, 1.0f);
				alignment = TextAlignment_Left;
				break;
			case ViewNameSnap_BottomRight:
				p1.Set((float)VIEW_SX, (float)VIEW_SY);
				dir.Set(-1.0f, -1.0f);
				alignment = TextAlignment_Left;
				break;
			default:
#if !defined(DEPLOYMENT)
				throw ExceptionVA("unknown view name snap");
#else
				break;
#endif
		}
		
		Vec2F p2a = p1 + Vec2F(dir[0] * sx1, dir[1] * 0.0f);
		Vec2F p2b = p1 + Vec2F(dir[0] * sx2, dir[1] * sy);
		Vec2F p1b = p1 + Vec2F(dir[0] * 0.0f, dir[1] * sy);
		
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::MENU_COLOR_WHITE);
		float u = image->m_Info->m_TexCoord[0];
		float v = image->m_Info->m_TexCoord[1];
		
		// render!

		g_GameState->DataSetActivate(image->m_TextureAtlas->m_DataSetId);
		
		SpriteGfx& gfx = *g_GameState->m_SpriteGfx;
		
		int rgba = SpriteColors::Black.rgba;
		
		gfx.Reserve(4, 6);
		gfx.WriteBegin();
		gfx.WriteVertex(p1[0], p1[1], rgba, u, v);
		gfx.WriteVertex(p2a[0], p2a[1], rgba, u, v);
		gfx.WriteVertex(p2b[0], p2b[1], rgba, u, v);
		gfx.WriteVertex(p1b[0], p1b[1], rgba, u, v);
		gfx.WriteIndex(0);
		gfx.WriteIndex(1);
		gfx.WriteIndex(2);
		gfx.WriteIndex(0);
		gfx.WriteIndex(2);
		gfx.WriteIndex(3);
		gfx.WriteEnd();
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE);
		
//		float tsx = font->m_Font.MeasureText(button->m_Name);
		
		RenderText(p2b, Vec2F(sx3 * dir[0], -sy * dir[1]), font, SpriteColors::White, alignment, TextAlignment_Center, true, button->m_Name);
	}

	void DesignApply(Menu* menu, DesignInfo* designInfoList, int designInfoCount)
	{
		for (int i = 0; i < designInfoCount; ++i)
		{
			DesignInfo* info = designInfoList + i;
			IGuiElement* element = menu->FindElement(info->mName);
			Assert(element != 0);
			Vec2F size = info->mSize.LengthSq_get() == 0.0f ? element->HitBox_get().m_Size : info->mSize;
			element->UpdateDesign(info->mIsVisible, info->mPosition, size);
		}
	}
}
