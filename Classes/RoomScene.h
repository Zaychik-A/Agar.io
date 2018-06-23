#pragma once

#include "cocos2d.h"

class Room : public cocos2d::Scene
{
public:
	static cocos2d::Scene* createScene();

	virtual bool init();

	//menu item callback
	void menuBackCallback(cocos2d::Ref* pSender);

	void menuItemReadyCallback(cocos2d::Ref* pSender);

	void menuItemCreateCallback(cocos2d::Ref* pSender);

	void menuItemJoinCallback(cocos2d::Ref* pSender);

	void update(float dt);

	// implement the "static create()" method manually
	CREATE_FUNC(Room);
};
