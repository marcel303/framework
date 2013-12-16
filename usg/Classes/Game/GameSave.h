#pragma once

namespace Game
{
	class GameSave
	{
	public:
		GameSave();
		
		void Save();
		void SaveUpdate();
		void Load();
		void Clear();
		
		bool HasSave_get() const;
	};
}
