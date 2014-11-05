#include "Client.h"
#include "Scene.h"

Client::Client(Engine* engine)
{
	FASSERT(engine);

	m_channel = 0;
	m_clientScene = new Scene(engine, 0);// FIXME. server/client side flag
	m_repID = 0;
	m_actionHandler = 0;
}

Client::~Client()
{
	delete m_clientScene;
}

void Client::Initialize(Channel* channel, bool clientSide)
{
	FASSERT(channel);

	m_channel = channel;
	m_clientSide = clientSide;
}

void Client::SetActionHandler(ActionHandler* handler)
{
	m_actionHandler = handler;
}
