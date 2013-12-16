#include "libgui_precompiled.h"

#if 0

#include "GuiNetworkRequest.h"
#include "GuiNetworkSubmit.h"
#include "GuiUtilActions.h"

namespace Gui
{
	namespace Util
	{
		namespace Actions
		{
			void Submit(Object* me, Object* sender)
			{
				Widget* widget = static_cast<Widget*>(me);
				Widget* target = static_cast<Widget*>(sender);

				Network::SubmitManager::I().SubmitToServer(target, widget->GetContext()->GetClientToServerChannel());
			}

			void Request(Object* me, Object* sender)
			{
				Widget* widget = static_cast<Widget*>(me);
				Widget* target = static_cast<Widget*>(sender);

				Network::RequestManager::I().Request(target, target->GetGlobalName(), widget->GetContext()->GetClientToServerChannel());
			}

			void Hide(Object* me, Object* sender)
			{
				//Widget* widget = static_cast<Widget*>(me);
				Widget* target = static_cast<Widget*>(sender);

				target->SetIsHidden(true);
			}

			void Show(Object* me, Object* sender)
			{
				//Widget* widget = static_cast<Widget*>(me);
				Widget* target = static_cast<Widget*>(sender);

				target->SetIsHidden(false);
			}

			void Destroy(Object* me, Object* sender)
			{
				//Widget* widget = static_cast<Widget*>(me);
				Widget* target = static_cast<Widget*>(sender);

				target->Destroy();
			}
		};
	};
};

#endif
