#import <UIKit/UIKit.h>

class Vec2
{
public:
	inline Vec2()
	{
		v[0] = v[1] = 0.0f;
	}
	
	inline Vec2(float x, float y)
	{
		v[0] = x;
		v[1] = y;
	}
	
	float Length_get() const
	{
		return sqrtf(v[0] * v[0] + v[1] * v[1]);
	}
	
	Vec2 Norm_get() const
	{
		float length = Length_get();
		
		if (length == 0.0f)
			return Vec2(1.0f, 0.0f);
		
		return Vec2(v[0] / length, v[1] / length);
	}
	
	float DistanceTo(Vec2 other) const
	{
		float dx = other[0] - v[0];
		float dy = other[1] - v[1];
		
		return sqrtf(dx * dx + dy * dy);
	}
	
	Vec2 OrientationTo(Vec2 other) const
	{
		Vec2 result = other.Subtract(*this);
		
		return result.Norm_get();
	}
	
	Vec2 Add(Vec2 other) const
	{
		return Vec2(v[0] + other[0], v[1] + other[1]);
	}
	
	Vec2 Subtract(Vec2 other) const
	{
		return Vec2(v[0] - other[0], v[1] - other[1]);
	}
	
	Vec2 Scale(float scale) const
	{
		return Vec2(v[0] * scale, v[1] * scale);
	}
	
	float ToAngle() const
	{
		return atan2f(v[1], v[0]);
	}
	
	static Vec2 FromAngle(float angle)
	{
		return Vec2(cosf(angle), sinf(angle));
	}
	
	float v[2];
	
	float& operator[](int index)
	{
		return v[index];
	}
	
	const float& operator[](int index) const
	{
		return v[index];
	}
};

#define DIS_ORIENT 25.0f
#define DIS_MOVE_AND_ORIENT 70.0f

class IController
{
public:
	virtual void TouchBegin(int finger, Vec2 pos) = 0;
	virtual void TouchMove(int finger, Vec2 pos) = 0;
	virtual void TouchEnd(int finger) = 0;
	
	virtual Vec2 Orientation_get() const = 0;
	virtual Vec2 MovementDirection_get() const = 0;
	virtual float MovementSpeed_get() const = 0;
	
	virtual const char* Name_get() const = 0;
};

class MyController1 : public IController
{
public:
	enum State
	{
		State_Idle,
		State_Noop,
		State_Orient,
		State_MoveAndOrient
	};
	
	MyController1()
	{
		Initialize();
	}
	
	void Initialize()
	{
		m_State = State_Idle;
		m_Pos = Vec2(160.0f, 240.0f);
		m_Finger = -1;
		
		m_MovementDistance = 0.0f;
	}
	
	void virtual TouchBegin(int finger, Vec2 pos)
	{
		if (m_Finger != -1)
			return;
		
		if (m_Pos.DistanceTo(pos) > DIS_ORIENT)
			return;
		
		m_Finger = finger;
		
		SetState(State_Noop);
	}
	
	void virtual TouchMove(int finger, Vec2 pos)
	{
		if (finger != m_Finger)
			return;
		
		// decide state based on distance
		
		float distance = m_Pos.DistanceTo(pos);
		
		switch (m_State)
		{
			case State_Noop:
				if (distance >= DIS_MOVE_AND_ORIENT)
					SetState(State_MoveAndOrient);
				else if (distance >= DIS_ORIENT)
					SetState(State_Orient);
				break;
				
			case State_Orient:
				if (distance >= DIS_MOVE_AND_ORIENT)
					SetState(State_MoveAndOrient);
				break;
				
			case State_MoveAndOrient:
				if (distance < DIS_MOVE_AND_ORIENT)
					SetState(State_Orient);
				break;
		}
		
		// update orientation and movement vector
		
		if (m_State == State_Orient || m_State == State_MoveAndOrient)
		{
			m_Orientation = m_Pos.OrientationTo(pos);
		}
		
		if (m_State == State_MoveAndOrient)
		{
			m_MovementDistance = distance - DIS_MOVE_AND_ORIENT;
			m_MovementVector = m_Orientation;
		}
	}
	
	void virtual TouchEnd(int finger)
	{
		if (finger != m_Finger)
			return;
		
		m_Finger = -1;
		
		SetState(State_Idle);
	}
	
	virtual Vec2 Orientation_get() const
	{
		return m_Orientation;
	}
	
	virtual Vec2 MovementDirection_get() const
	{
		return m_MovementVector;
	}
	
	virtual float MovementSpeed_get() const
	{
		return m_MovementDistance / 50.0f * 200.0f;
	}
	
	virtual const char* Name_get() const
	{
		return "UniFinger";
	}
	
	void SetState(State state)
	{
		switch (state)
		{
			case State_Idle:
				m_MovementVector = Vec2(0.0f, 0.0f);
				break;
				
			case State_Orient:
				m_MovementVector = Vec2(0.0f, 0.0f);
				break;
		}
		
		m_State = state;
	}
	
	State m_State;
	Vec2 m_Pos;
	int m_Finger;
	
	//
	
	Vec2 m_Orientation;
	Vec2 m_MovementVector;
	float m_MovementDistance;
};

class Analog
{
public:
	enum Mode
	{
		Mode_Relative,
		Mode_Absolute
	};
	
	Analog()
	{
		m_Radius = 0.0f;
		m_Finger = -1;
		m_Direction = Vec2(1.0f, 0.0f);
	}
	
	void Setup(Mode mode, Vec2 pos, float radius, float deadZone)
	{
		m_Mode = mode;
		m_Pos = pos;
		m_Radius = radius;
		m_DeadZone = deadZone;
	}
	
	void TouchBegin(int finger, Vec2 pos)
	{
		// is inside circle?
		
		float distance = m_Pos.DistanceTo(pos) - m_Radius;
		
		if (distance > 0.0f)
			return;
		
		m_Finger = finger;
		m_StartPos = pos;
		m_CurrentPos = pos;
	}
	
	void TouchMove(int finger, Vec2 pos)
	{
		if (finger != m_Finger)
			return;
		
		float distance = m_Pos.DistanceTo(pos) - m_DeadZone;
		
		m_CurrentPos = pos;
		
		if (distance > 0.0f)
		{
			m_Direction = m_StartPos.OrientationTo(m_CurrentPos);
		}
	}
	
	void TouchEnd(int finger)
	{
		if (finger != m_Finger)
			return;
		
		m_Finger = -1;
	}
	
	Vec2 Direction_get() const
	{
		return m_Direction;
	}
	
	float Distance_get() const
	{
		if (m_Finger == -1)
			return 0.0f;
		
		return m_StartPos.DistanceTo(m_CurrentPos) / m_Radius;
	}
	
	Mode m_Mode;
	Vec2 m_Pos;
	float m_Radius;
	float m_DeadZone;
	
	int m_Finger;
	Vec2 m_StartPos;
	Vec2 m_CurrentPos;
	Vec2 m_Direction;
};

class MyController2 : public IController
{
public:
	MyController2()
	{
		Initialize();
	}
	
	void Initialize()
	{
		float size = 50.0f;
		float border = size + 20.0f;
		
		m_Analog1.Setup(Analog::Mode_Absolute, Vec2(320.0f - border, 480.0f - border), size, 0.0f);
		m_Analog2.Setup(Analog::Mode_Relative, Vec2(320.0f - border, border), size, 0.0f);
	}
	
	virtual void TouchBegin(int finger, Vec2 pos)
	{
		m_Analog1.TouchBegin(finger, pos);
		m_Analog2.TouchBegin(finger, pos);
	}
	
	virtual void TouchMove(int finger, Vec2 pos)
	{
		m_Analog1.TouchMove(finger, pos);
		m_Analog2.TouchMove(finger, pos);
	}
	
	virtual void TouchEnd(int finger)
	{
		m_Analog1.TouchEnd(finger);
		m_Analog2.TouchEnd(finger);
	}
	
	virtual Vec2 Orientation_get() const
	{
		return m_Analog2.Direction_get();
	}
	
	virtual Vec2 MovementDirection_get() const
	{
		return m_Analog1.Direction_get();
	}
		
	virtual float MovementSpeed_get() const
	{
		return m_Analog1.Distance_get() * 200.0f;
	}
	
	virtual const char* Name_get() const
	{
		return "MultiFinger1";
	}
	
	Analog m_Analog1;
	Analog m_Analog2;
};

class MyController3 : public IController
{
public:
	MyController3()
	{
		Initialize();
	}
	
	void Initialize()
	{
		float size = 50.0f;
		float border = size + 20.0f;
		
		m_Analog1.Setup(Analog::Mode_Absolute, Vec2(320.0f - border, 480.0f - border), size, 0.0f);
		m_Analog2.Setup(Analog::Mode_Relative, Vec2(320.0f - border, border), size, 0.0f);
	}
	
	virtual void TouchBegin(int finger, Vec2 pos)
	{
		m_Analog1.TouchBegin(finger, pos);
		m_Analog2.TouchBegin(finger, pos);
	}
	
	virtual void TouchMove(int finger, Vec2 pos)
	{
		m_Analog1.TouchMove(finger, pos);
		m_Analog2.TouchMove(finger, pos);
	}
	
	virtual void TouchEnd(int finger)
	{
		m_Analog1.TouchEnd(finger);
		m_Analog2.TouchEnd(finger);
	}
	
	virtual Vec2 Orientation_get() const
	{
		return m_Analog2.Direction_get();
	}
	
	virtual Vec2 MovementDirection_get() const
	{
		return Orientation_get();
	}
		
	virtual float MovementSpeed_get() const
	{
		if (m_Analog1.m_Finger == -1)
			return 0.0f;
		
		return (m_Analog1.m_StartPos[0] - m_Analog1.m_CurrentPos[0]) / m_Analog1.m_Radius * 200.0f;
	}
	
	virtual const char* Name_get() const
	{
		return "MultiFinger2";
	}
	
	Analog m_Analog1;
	Analog m_Analog2;
};

@interface MainView : UIView {
	MyController1 m_Controller1;
	MyController2 m_Controller2;
	MyController3 m_Controller3;
	
	IController* m_ActiveController;
	
	Vec2 m_Pos;
}

- (void)update;
- (void)toggle;
- (void)select:(IController*)controller;
- (void)benchmark;

@end
