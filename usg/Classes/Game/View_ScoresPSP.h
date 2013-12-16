#pragma once

#include "AnimTimer.h"
#include "GameSettings.h"
#include "IView.h"
#include "Types.h"

#include <algorithm>
#include <map>
#include "FixedSizeString.h"
#include "PolledTimer.h"
#include "ScreenLock.h"
#include "StreamReader.h"
#include "StreamWriter.h"

namespace Game
{
	/*

	PSP score board:

	- per gamemode
	- per difficulty
	- list of top N scores

	select through game modes using prev/next
	select throw difficulty using prev/next

	capture digital left/right button input to cycle through lists

	on focus, select gamemode/difficulty

	load/save system:
	- load scores in ctor
	- save scores after insert (all gamemodes/difficulties stored in 1 file)
	- view: filter scores

	*/

	const static int kMaxPspScores = 16;

	class PspScore
	{
	public:
		PspScore() : Score(-1) { }
		PspScore(int score, const char* name) : Score(score), Name(name) { }

		void Load(StreamReader& reader)
		{
			Score = reader.ReadInt32();
			Name = reader.ReadText_Binary().c_str();
		}

		void Save(StreamWriter& writer) const
		{
			writer.WriteInt32(Score);
			writer.WriteText_Binary(Name.c_str());
		}

		bool operator <(const PspScore& other) const
		{
			if (Score > other.Score)
				return true;
			if (Score < other.Score)
				return false;

			return false;
		}
		
		int Score;
		FixedSizeString<32> Name;
	};

	class PspScoreList
	{
	public:
		PspScoreList() : ScoreCount(0) { }

		void Load(StreamReader& reader)
		{
			const int scoreCount = reader.ReadInt32();

			// load up ontil kMaxPspScores score entries

			if (scoreCount > kMaxPspScores)
				ScoreCount = kMaxPspScores;
			else
				ScoreCount = scoreCount;

			for (int i = 0; i < ScoreCount; ++i)
			{
				ScoreList[i].Load(reader);
			}

			std::stable_sort(ScoreList, ScoreList + ScoreCount);
			
			// discard any other scores

			for (int i = ScoreCount; i < scoreCount; ++i)
			{
				PspScore score;
				score.Load(reader);
			}
		}

		void Save(StreamWriter& writer) const
		{
			writer.WriteInt32(ScoreCount);

			for (int i = 0; i < ScoreCount; ++i)
			{
				ScoreList[i].Save(writer);
			}
		}

		void Add(const PspScore& score)
		{
			ScoreList[ScoreCount] = score;

			std::stable_sort(ScoreList, ScoreList + ScoreCount + 1);

			if (ScoreCount < kMaxPspScores)
				ScoreCount++;
		}

		void Clear()
		{
			ScoreCount = 0;
		}

		int ScoreCount;
		PspScore ScoreList[kMaxPspScores + 1];
	};

	class PspScoreKey
	{
	public:
		PspScoreKey() : GameMode(-1), Difficulty(Difficulty_Unknown) { }
		PspScoreKey(int gameMode, Difficulty difficulty) : GameMode(gameMode), Difficulty(difficulty) { }

		void Load(StreamReader& reader)
		{
			GameMode = reader.ReadInt32();
			Difficulty = (::Difficulty)reader.ReadInt32();
		}

		void Save(StreamWriter& writer) const
		{
			writer.WriteInt32(GameMode);
			writer.WriteInt32(Difficulty);
		}

		bool operator <(const PspScoreKey& other) const
		{
			if (GameMode < other.GameMode)
				return true;
			if (GameMode > other.GameMode)
				return false;

			if (Difficulty < other.Difficulty)
				return true;
			if (Difficulty > other.Difficulty)
				return false;

			return false;
		}

		int GameMode;
		Difficulty Difficulty;
	};
	
	class PspScoreDB
	{
	public:
		~PspScoreDB()
		{
			Clear();
		}

		void Clear()
		{
			for (std::map<PspScoreKey, PspScoreList*>::iterator i = ScoresByKey.begin(); i != ScoresByKey.end(); ++i)
				delete i->second;
			ScoresByKey.clear();
		}

		PspScoreList* Get(const PspScoreKey& key)
		{
			std::map<PspScoreKey, PspScoreList*>::iterator i = ScoresByKey.find(key);

			if (i != ScoresByKey.end())
				return i->second;

			PspScoreList* list = new PspScoreList();

			ScoresByKey[key] = list;

			return list;
		}

		void Load(StreamReader& reader)
		{
			const int listCount = reader.ReadInt32();

			for (int i = 0; i < listCount; ++i)
			{
				PspScoreKey key;

				key.Load(reader);

				PspScoreList* list = Get(key);

				list->Clear();

				list->Load(reader);

				ScoresByKey[key] = list;
			}
		}

		void Save(StreamWriter& writer) const
		{
			writer.WriteInt32((int)ScoresByKey.size());

			for (std::map<PspScoreKey, PspScoreList*>::const_iterator i = ScoresByKey.begin(); i != ScoresByKey.end(); ++i)
			{
				const PspScoreKey& key = i->first;
				const PspScoreList* list = i->second;

				key.Save(writer);

				list->Save(writer);
			}
		}
		
		std::map<PspScoreKey, PspScoreList*> ScoresByKey;
	};

	class PspScoreScroller
	{
	public:
		PspScoreScroller();

		void Update(float dt);
		void Reset();

		float Progress_get() const;

	private:
		enum State
		{
			State_ScrollDownWait,
			State_ScrollDown,
			State_ScrollUpWait,
			State_ScrollUp,
			
		};

		void State_set(State state);

		State mState;
		PolledTimer mWaitTimer;
		AnimTimer mScrollTimer;
		float mProgress;
	};
	
	class View_ScoresPSP : public IView
	{
	public:
		View_ScoresPSP();
		~View_ScoresPSP();
		void Initialize();
		
		void Load();
		void Save();
		void Submit(int gameMode, Difficulty difficulty, int score, const char* name, bool save = true);
		
		void Show(int gameMode, Difficulty difficulty);

		void ChangeGameMode(int gameMode, bool refresh);
		void ChangeDifficulty(Difficulty difficulty, bool refresh);
		void Refresh();

	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		// --------------------
		// View
		// --------------------
		virtual float FadeTime_get();
		virtual void HandleFocus();
		virtual void HandleFocusLost();

		PspScoreDB* mScoreDB;
		int mSelectedGameMode;
		Difficulty mSelectedDifficulty;
		PspScoreList* mSelectedScoreListPrev;
		PspScoreList* mSelectedScoreList;
		int mAnimState;
		AnimTimer mAnimTimer;
		PspScoreScroller mScoreScroller;
		AnimTimer mScoreBlinkTimer;
		ScreenLock mScreenLock;
		AnimTimer mFadeTimer;
	};
	
	//
}
