#pragma once

#include "GraphicsI.h"
#include "Log.h"

template <int BLOCK_SIZE, int SIZE>
class BlockAllocator
{
public:
	BlockAllocator()
	{
		mBytes = 0;
		memset(mBlockIsUsed, 0, sizeof(mBlockIsUsed));
		memset(mAllocSize, 0, sizeof(mAllocSize));
		mAllocSizeTotal = 0;
	}

	void Setup(void* bytes)
	{
		mBytes = (uint8_t*)bytes;
	}

	void* Alloc(int size)
	{
		const int allocSize = Calc::DivideUp(size, BLOCK_SIZE);

		for (int i = 0; i <= COUNT - allocSize; ++i)
		{
			bool found = true;

			for (int j = 0; j < allocSize; ++j)
			{
				if (mBlockIsUsed[i + j])
				{
					found = false;
				}
			}

			if (found)
			{
				for (int j = 0; j < allocSize; ++j)
				{
					mBlockIsUsed[i + j] = true;
					Assert(mAllocSize[i + j] == 0);
				}

				mAllocSize[i] = allocSize;
				mAllocSizeTotal += allocSize;

				LOG_DBG("%s: allocated %d bytes. %d bytes requested. total alloc size: %d bytes", "PspVram", allocSize * BLOCK_SIZE, size, mAllocSizeTotal * BLOCK_SIZE);

				return mBytes + i * BLOCK_SIZE;
			}
		}

		return 0;
	}

	void Free(void* p)
	{
		const int blockIdx = (((uint8_t*)p) - mBytes) / BLOCK_SIZE;

		Assert(mBytes + BLOCK_SIZE * blockIdx == p);
		Assert(mBlockIsUsed[blockIdx]);

		const int allocSize = mAllocSize[blockIdx];

		Assert(allocSize != 0);

		for (int i = 0; i < allocSize; ++i)
		{
			Assert(mBlockIsUsed[blockIdx + i]);
			mBlockIsUsed[blockIdx + i] = false;
			mAllocSize[blockIdx + i] = 0;
		}

		mAllocSizeTotal -= allocSize;

		LOG_DBG("%s: freed %d bytes. total alloc size: %d bytes", "PspVram", allocSize * BLOCK_SIZE, mAllocSizeTotal * BLOCK_SIZE);
	}

private:
	const static int COUNT = SIZE / BLOCK_SIZE;

	uint8_t* mBytes;
	bool mBlockIsUsed[COUNT];
	int mAllocSize[COUNT];
	int mAllocSizeTotal;
};

class Graphics_Psp : public GraphicsI
{
public:
	Graphics_Psp();
	virtual ~Graphics_Psp();

	virtual void Initialize(int sx, int sy);
	virtual void Shutdown();

	virtual void MakeCurrent();
	virtual void Clear(float r, float g, float b, float a);
	virtual void BlendModeSet(BlendMode blendMode);
	virtual void MatrixSet(MatrixType type, const Mat4x4& mat);
	virtual void MatrixPush(MatrixType type);
	virtual void MatrixPop(MatrixType type);
	virtual void MatrixTranslate(MatrixType type, float x, float y, float z);
	virtual void MatrixRotate(MatrixType type, float angle, float x, float y, float z);
	virtual void MatrixScale(MatrixType type, float x, float y, float z);
	virtual void TextureSet(Res* texture);
	virtual void TextureIsEnabledSet(bool value);
	virtual void TextureDestroy(Res* res);
	virtual void TextureCreate(Res* res);
	virtual void Present();

	//

	void FinishAndRestart();
	void ManualSwap();
	void Block();
	void AsyncSwapEnabled_set(bool enabled);

	bool mRenderInProgress;

private:
	void Finish(bool asyncSwap);

	int mTextureId;
	BlockAllocator<1024 * 16, 1024 * 1024> mVramAllocator;
	bool mAsyncSwapEnabled;
	LogCtx mLog;
};
