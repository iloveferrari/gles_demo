#ifndef INPUT_H
#define INPUT_H

#include <unordered_map>
#include <set>
using namespace std;

enum KEYNAME
{
	LEFT_CLICK,
	RIGHT_CLICK,
	MIDDLE_CLICK,
};

enum DIRECTION
{
	NOINPUT,
	FORWARD,
	LEFT,
	BACK,
	RIGHT,
	UP,
	DOWN,
};

class Input;

static Input *instance = nullptr;

class Input
{
public:
	Input()
		: m_x(0)
		, m_y(0)
		, m_sensitivity(0.02f)
		, m_mouseWheelScrollDis(0.0f)
	{}

	static Input *getInstance()
	{
		if (instance == nullptr)
		{
			instance = new Input();
		}

		return instance;
	}

	void updateAxis(int x, int y)
	{
		m_detalX = x - m_x;
		m_detalY = y - m_y;
		m_x = x;
		m_y = y;
		//printf("m_detalX %f\n", m_detalX);
		//printf("m_detalY %f\n", m_detalY);
	}

	void updateKeys(KEYNAME keyName, bool state)
	{
		m_keysState[keyName] = state;
	}

	void updateMoveDirection(DIRECTION dir)
	{
		m_direction = dir;
	}

	void updateMouseWheelScroll(float scrollDis)
	{
		m_mouseWheelScrollDis = scrollDis;
	}

	float getMouseWheelScroll()
	{
		float result = m_mouseWheelScrollDis;
		m_mouseWheelScrollDis = 0;
		return result;
	}

	DIRECTION getMoveDirection()
	{
		return m_direction;
	}

	bool getKeyState(KEYNAME keyName)
	{
		return m_keysState[keyName];
	}

	float getAxisX()
	{
		int result = m_detalX;
		m_detalX = 0;
		return result;
	}

	float getAxisY()
	{
		int result = m_detalY;
		m_detalY = 0;
		return result;
	}

private:
	int m_x, m_y;
	float m_detalX, m_detalY;
	float m_sensitivity;
	unordered_map<KEYNAME, bool> m_keysState;
	DIRECTION m_direction;
	float m_mouseWheelScrollDis;
};

#endif INPUT_H