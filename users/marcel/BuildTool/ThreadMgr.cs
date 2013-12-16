using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace BuildTool
{
	public class Task
	{
		public delegate bool TaskHandler(object argument);
		private object mArgument;
		private TaskHandler mTaskHandler;

		public Task(TaskHandler handler, object argument)
		{
			mTaskHandler = handler;
			mArgument = argument;
		}

		public void Start(TaskMgr mgr)
		{
			Thread thread = new Thread(Execute);

			thread.Start(mgr);
		}

		private void Execute(object obj)
		{
			TaskMgr mgr = (TaskMgr)obj;

			try
			{
				bool result = mTaskHandler(mArgument);

				if (result == false)
				{
					mgr.HandleError();
				}
			}
			catch (Exception e)
			{
				Console.WriteLine("error: {0}", e.Message);

				mgr.HandleError();
			}
			finally
			{
				mgr.HandleFinish();
			}
		}
	}

	public class TaskMgr
	{
		private Stack<Task> mTaskList = new Stack<Task>();
		private AutoResetEvent mReset = new AutoResetEvent(false);
		private int mActiveThreadCount = 0;
		private bool mFinish = false;
		private AutoResetEvent mFinishReset = new AutoResetEvent(false);
		private AutoResetEvent mStartReset = new AutoResetEvent(false);
		private bool mBailOnErrors = false;
		private bool mHadErrors = false;
		private bool mIsRunning = false;

		public void Add(Task task)
		{
			lock (this)
			{
				mTaskList.Push(task);
			}

			mReset.Set();
		}

		public void Start(bool bailOnErrors)
		{
			System.Diagnostics.Debug.Assert(mIsRunning == false);

			mFinish = false;
			mBailOnErrors = bailOnErrors;
			mHadErrors = false;

			Thread thread = new Thread(Execute);

			thread.Start();

			mStartReset.WaitOne();

			System.Diagnostics.Debug.Assert(mIsRunning == true);
		}

		public void Finish()
		{
			System.Diagnostics.Debug.Assert(mIsRunning == true);

			mFinish = true;

			mReset.Set();
		}

		public void WaitForFinish()
		{
			System.Diagnostics.Debug.Assert(mIsRunning == true);

			Finish();

			mFinishReset.WaitOne();

			System.Diagnostics.Debug.Assert(mIsRunning == false);
		}

		private void Execute(object obj)
		{
			mIsRunning = true;

			mStartReset.Set();

			while (!mFinish || mActiveThreadCount > 0)
			{
				//Console.WriteLine("check");

				if (mBailOnErrors && mHadErrors)
				{
					lock (this)
					{
						mTaskList.Clear();
					}
				}

				if (mTaskList.Count > 0 && mActiveThreadCount < 6)
				{
					Task task;

					lock (this)
					{
						task = mTaskList.Pop();

						mActiveThreadCount++;
					}

					task.Start(this);
				}
				else
				{
					mReset.WaitOne();
				}
			}

			mIsRunning = false;

			mFinishReset.Set();
		}

		public void HandleFinish()
		{
			lock (this)
			{
				mActiveThreadCount--;
			}

			mReset.Set();
		}

		public void HandleError()
		{
			lock (this)
			{
				mHadErrors = true;
			}
		}

		public bool HadErrors()
		{
			lock (this)
			{
				return mHadErrors;
			}
		}

		public bool IsRunning
		{
			get
			{
				return mIsRunning;
			}
		}
	}
}
