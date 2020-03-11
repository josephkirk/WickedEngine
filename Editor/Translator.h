#pragma once
#include "CommonInclude.h"

#include <list>

class Translator
{
private:
	XMFLOAT4 prevPointer;
	XMFLOAT4X4 dragStart, dragEnd;
	bool dragging;
	bool dragStarted;
	bool dragEnded;
public:
	Translator();

	void Update();
	void Draw(const wiScene::CameraComponent& camera, wiGraphics::CommandList cmd) const;

	// Attach selection to translator temporarily
	void PreTranslate();
	// Apply translator to selection
	void PostTranslate();

	wiScene::TransformComponent transform;
	std::list<wiScene::PickResult> selected;

	bool enabled;

	enum TRANSLATOR_STATE
	{
		TRANSLATOR_IDLE,
		TRANSLATOR_X,
		TRANSLATOR_Y,
		TRANSLATOR_Z,
		TRANSLATOR_XY,
		TRANSLATOR_XZ,
		TRANSLATOR_YZ,
		TRANSLATOR_XYZ,
	} state;

	float dist;

	bool isTranslator, isScalator, isRotator;


	// Check if the drag started in this exact frame
	bool IsDragStarted();
	XMFLOAT4X4 GetDragStart();
	// Check if the drag ended in this exact frame
	bool IsDragEnded();
	XMFLOAT4X4 GetDragEnd();

	static void LoadShaders();
};

