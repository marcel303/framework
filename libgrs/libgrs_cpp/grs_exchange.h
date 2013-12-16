#pragma once

#include <string>
#include "CallBack.h"

namespace GRS
{
	class Exchange
	{
	public:
		Exchange();
		~Exchange();
		
		void Post(const std::string& url, const std::string& httpBody);
		void Post_Synchronized(const std::string& url, const std::string& httpBody);
		
		CallBack OnResult;
		
	private:
		static void HandleResponse(void* obj, void* arg);
		
		void* requestHelper;
	};
}
