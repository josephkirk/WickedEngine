#include "wiWidget.h"
#include "wiGUI.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiFont.h"
#include "wiMath.h"
#include "wiHelper.h"
#include "wiInputManager.h"

#include <DirectXCollision.h>

#include <sstream>

using namespace std;
using namespace wiGraphicsTypes;

static GraphicsPSO* PSO_colorpicker = nullptr;


wiWidget::wiWidget()
{
	state = IDLE;
	enabled = true;
	visible = true;
	colors[IDLE] = wiColor::Booger;
	colors[FOCUS] = wiColor::Gray;
	colors[ACTIVE] = wiColor::White;
	colors[DEACTIVATING] = wiColor::Gray;
	scissorRect.bottom = 0;
	scissorRect.left = 0;
	scissorRect.right = 0;
	scissorRect.top = 0;
	container = nullptr;
	tooltipTimer = 0;
	textColor = wiColor(255, 255, 255, 255);
	textShadowColor = wiColor(0, 0, 0, 255);
}
wiWidget::~wiWidget()
{
}
void wiWidget::Update(wiGUI* gui, float dt )
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (GetState() == WIDGETSTATE::FOCUS && !tooltip.empty())
	{
		tooltipTimer++;
	}
	else
	{
		tooltipTimer = 0;
	}

	wiECS::Entity entityID = (wiECS::Entity)fastName.GetHash();
	auto ref = gui->transforms.Find(entityID);
	if (gui->transforms.IsValid(ref, entityID))
	{
		const auto& transform = gui->transforms.GetComponent(ref);
		hitBox.pos.x = transform.translation.x;
		hitBox.pos.y = transform.translation.y;
		hitBox.siz.x = transform.scale.x;
		hitBox.siz.y = transform.scale.y;
	}
	else
	{
		auto ref = gui->transforms.Create(entityID);
		auto& transform = gui->transforms.GetComponent(ref);
		transform.translation_local.x = hitBox.pos.x;
		transform.translation_local.y = hitBox.pos.y;
		transform.scale_local.x = hitBox.siz.x;
		transform.scale_local.y = hitBox.siz.y;

		XMVECTOR scale_local = XMLoadFloat3(&transform.scale_local);
		XMVECTOR rotation_local = XMLoadFloat4(&transform.rotation_local);
		XMVECTOR translation_local = XMLoadFloat3(&transform.translation_local);
		XMMATRIX world =
			XMMatrixScalingFromVector(scale_local) *
			XMMatrixRotationQuaternion(rotation_local) *
			XMMatrixTranslationFromVector(translation_local);
		XMStoreFloat4x4(&transform.world, world);

		if (container != nullptr)
		{
			wiECS::Entity entityID_parent = (wiECS::Entity)fastName.GetHash();
			auto ref_parent = gui->transforms.Find(entityID_parent);
			const auto& transform_parent = gui->transforms.GetComponent(ref_parent);

			XMMATRIX world_parent = XMLoadFloat4x4(&transform_parent.world);
			XMMATRIX world_parent_inv = XMMatrixInverse(nullptr, world_parent);
			XMStoreFloat4x4(&transform.world_parent_bind, world_parent_inv);

			transform.parent_ref = ref_parent;
		}
	}
}
void wiWidget::RenderTooltip(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (tooltipTimer > 25)
	{
		gui->ResetScissor();

		XMFLOAT2 tooltipPos = XMFLOAT2(gui->pointerpos.x, gui->pointerpos.y);
		if (tooltipPos.y > wiRenderer::GetDevice()->GetScreenHeight()*0.8f)
		{
			tooltipPos.y -= 30;
		}
		else
		{
			tooltipPos.y += 40;
		}
		wiFontProps fontProps = wiFontProps((int)tooltipPos.x, (int)tooltipPos.y, -1, WIFALIGN_LEFT, WIFALIGN_TOP);
		fontProps.color = wiColor(25, 25, 25, 255);
		wiFont tooltipFont = wiFont(tooltip, fontProps);
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(tooltip + "\n" + scriptTip);
		}
		static const float _border = 2;
		float fontWidth = (float)tooltipFont.textWidth() + _border* 2;
		float fontHeight = (float)tooltipFont.textHeight() + _border * 2;
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(wiColor(255, 234, 165)), wiImageEffects(tooltipPos.x - _border, tooltipPos.y - _border, fontWidth, fontHeight), gui->GetGraphicsThread());
		tooltipFont.SetText(tooltip);
		tooltipFont.Draw(gui->GetGraphicsThread());
		if (!scriptTip.empty())
		{
			tooltipFont.SetText(scriptTip);
			tooltipFont.props.posY += (int)(fontHeight / 2);
			tooltipFont.props.color = wiColor(25, 25, 25, 110);
			tooltipFont.Draw(gui->GetGraphicsThread());
		}
	}
}
wiHashString wiWidget::GetName()
{
	return fastName;
}
void wiWidget::SetName(const std::string& value)
{
	if (value.empty())
	{
		static unsigned long widgetID = 0;
		stringstream ss("");
		ss << "widget_" << widgetID++;
		fastName = wiHashString(ss.str());
	}
	else
	{
		fastName = wiHashString(value);
	}
}
string wiWidget::GetText()
{
	return text;
}
void wiWidget::SetText(const std::string& value)
{
	text = value;
}
void wiWidget::SetTooltip(const std::string& value)
{
	tooltip = value;
}
void wiWidget::SetScriptTip(const std::string& value)
{
	scriptTip = value;
}
void wiWidget::SetPos(const XMFLOAT2& value)
{
	hitBox.pos = value;
}
void wiWidget::SetSize(const XMFLOAT2& value)
{
	hitBox.siz = value;
}
wiWidget::WIDGETSTATE wiWidget::GetState()
{
	return state;
}
void wiWidget::SetEnabled(bool val) 
{ 
	enabled = val; 
}
bool wiWidget::IsEnabled() 
{ 
	return enabled && visible; 
}
void wiWidget::SetVisible(bool val)
{ 
	visible = val;
}
bool wiWidget::IsVisible() 
{ 
	return visible;
}
void wiWidget::Activate()
{
	state = ACTIVE;
}
void wiWidget::Deactivate()
{
	state = DEACTIVATING;
}
void wiWidget::SetColor(const wiColor& color, WIDGETSTATE state)
{
	if (state == WIDGETSTATE_COUNT)
	{
		for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
		{
			colors[i] = color;
		}
	}
	else
	{
		colors[state] = color;
	}
}
wiColor wiWidget::GetColor()
{
	wiColor retVal = colors[GetState()];
	if (!IsEnabled()) {
		retVal = wiColor::lerp(wiColor::Transparent, retVal, 0.5f);
	}
	return retVal;
}
void wiWidget::SetScissorRect(const wiGraphicsTypes::Rect& rect)
{
	scissorRect = rect;
	if(scissorRect.bottom>0)
		scissorRect.bottom -= 1;
	if (scissorRect.left>0)
		scissorRect.left += 1;
	if (scissorRect.right>0)
		scissorRect.right -= 1;
	if (scissorRect.top>0)
		scissorRect.top += 1;
}
void wiWidget::LoadShaders()
{

	{
		GraphicsPSODesc desc;
		desc.vs = wiRenderer::vertexShaders[VSTYPE_LINE];
		desc.ps = wiRenderer::pixelShaders[PSTYPE_LINE];
		desc.il = wiRenderer::vertexLayouts[VLTYPE_LINE];
		desc.dss = wiRenderer::depthStencils[DSSTYPE_XRAY];
		desc.bs = wiRenderer::blendStates[BSTYPE_OPAQUE];
		desc.rs = wiRenderer::rasterizers[RSTYPE_DOUBLESIDED];
		desc.numRTs = 1;
		desc.RTFormats[0] = wiRenderer::GetDevice()->GetBackBufferFormat();
		desc.pt = TRIANGLESTRIP;
		RECREATE(PSO_colorpicker);
		HRESULT hr = wiRenderer::GetDevice()->CreateGraphicsPSO(&desc, PSO_colorpicker);
		assert(SUCCEEDED(hr));
	}
}


wiButton::wiButton(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	OnDragStart([](wiEventArgs args) {});
	OnDrag([](wiEventArgs args) {});
	OnDragEnd([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));
}
wiButton::~wiButton()
{

}
void wiButton::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		onDragEnd(args);

		if (pointerHitbox.intersects(hitBox))
		{
			// Click occurs when the button is released within the bounds
			onClick(args);
		}

		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);

			wiEventArgs args;
			args.clickPos = pointerHitbox.pos;
			XMFLOAT3 posDelta;
			posDelta.x = pointerHitbox.pos.x - prevPos.x;
			posDelta.y = pointerHitbox.pos.y - prevPos.y;
			posDelta.z = 0;
			args.deltaPos = XMFLOAT2(posDelta.x, posDelta.y);
			onDrag(args);
		}
	}

	if (clicked)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		dragStart = args.clickPos;
		args.startPos = dragStart;
		onDragStart(args);
		gui->ActivateWidget(this);
	}

	prevPos.x = pointerHitbox.pos.x;
	prevPos.y = pointerHitbox.pos.y;

}
void wiButton::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	gui->ResetScissor();

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());



	scissorRect.bottom = (LONG)(hitBox.pos.y + hitBox.siz.y);
	scissorRect.left = (LONG)(hitBox.pos.x);
	scissorRect.right = (LONG)(hitBox.pos.x + hitBox.siz.x);
	scissorRect.top = (LONG)(hitBox.pos.y);
	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps((int)(hitBox.pos.x + hitBox.siz.x*0.5f), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1, 
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread());

}
void wiButton::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = std::move(func);
}
void wiButton::OnDragStart(function<void(wiEventArgs args)> func)
{
	onDragStart = move(func);
}
void wiButton::OnDrag(function<void(wiEventArgs args)> func)
{
	onDrag = move(func);
}
void wiButton::OnDragEnd(function<void(wiEventArgs args)> func)
{
	onDragEnd = move(func);
}




wiLabel::wiLabel(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(100, 20));
}
wiLabel::~wiLabel()
{

}
void wiLabel::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiLabel::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	gui->ResetScissor();

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());


	scissorRect.bottom = (LONG)(hitBox.pos.y + hitBox.siz.y);
	scissorRect.left = (LONG)(hitBox.pos.x);
	scissorRect.right = (LONG)(hitBox.pos.x + hitBox.siz.x);
	scissorRect.top = (LONG)(hitBox.pos.y);
	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps((int)hitBox.pos.x + 2, (int)hitBox.pos.y + 2, -1, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1, 
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread());

}




string wiTextInputField::value_new = "";
wiTextInputField::wiTextInputField(const std::string& name) :wiWidget()
{
	SetName(name);
	SetText(fastName.GetString());
	OnInputAccepted([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 30));
}
wiTextInputField::~wiTextInputField()
{

}
void wiTextInputField::SetValue(const std::string& newValue)
{
	value = newValue;
}
void wiTextInputField::SetValue(int newValue)
{
	stringstream ss("");
	ss << newValue;
	value = ss.str();
}
void wiTextInputField::SetValue(float newValue)
{
	stringstream ss("");
	ss << newValue;
	value = ss.str();
}
const std::string& wiTextInputField::GetValue()
{
	return value;
}
void wiTextInputField::Update(wiGUI* gui, float dt)
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));
	bool intersectsPointer = pointerHitbox.intersects(hitBox);

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	bool clicked = false;
	// hover the button
	if (intersectsPointer)
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	if (clicked)
	{
		gui->ActivateWidget(this);

		value_new = value;
	}

	if (state == ACTIVE)
	{
		if (wiInputManager::GetInstance()->press(VK_RETURN, wiInputManager::KEYBOARD))
		{
			// accept input...

			value = value_new;
			value_new.clear();

			wiEventArgs args;
			args.sValue = value;
			args.iValue = atoi(value.c_str());
			args.fValue = (float)atof(value.c_str());
			onInputAccepted(args);

			gui->DeactivateWidget(this);
		}
		else if ((wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD) && !intersectsPointer) ||
			wiInputManager::GetInstance()->press(VK_ESCAPE, wiInputManager::KEYBOARD))
		{
			// cancel input 
			value_new.clear();
			gui->DeactivateWidget(this);
		}

	}

}
void wiTextInputField::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	gui->ResetScissor();

	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());



	scissorRect.bottom = (LONG)(hitBox.pos.y + hitBox.siz.y);
	scissorRect.left = (LONG)(hitBox.pos.x);
	scissorRect.right = (LONG)(hitBox.pos.x + hitBox.siz.x);
	scissorRect.top = (LONG)(hitBox.pos.y);
	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());

	string activeText = text;
	if (state == ACTIVE)
	{
		activeText = value_new;
	}
	else if (!value.empty())
	{
		activeText = value;
	}
	wiFont(activeText, wiFontProps((int)(hitBox.pos.x + 2), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_LEFT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread());

}
void wiTextInputField::OnInputAccepted(function<void(wiEventArgs args)> func)
{
	onInputAccepted = move(func);
}
void wiTextInputField::AddInput(const char inputChar)
{
	value_new.push_back(inputChar);
}
void wiTextInputField::DeleteFromInput()
{
	if (!value_new.empty())
	{
		value_new.pop_back();
	}
}





wiSlider::wiSlider(float start, float end, float defaultValue, float step, const std::string& name) :wiWidget()
	,start(start), end(end), value(defaultValue), step(max(step, 1))
{
	SetName(name);
	SetText(fastName.GetString());
	OnSlide([](wiEventArgs args) {});
	SetSize(XMFLOAT2(200, 40));

	valueInputField = new wiTextInputField(name + "_endInputField");
	valueInputField->SetSize(XMFLOAT2(hitBox.siz.y * 2, hitBox.siz.y));
	valueInputField->SetPos(XMFLOAT2(hitBox.siz.x + 20, 0));
	valueInputField->SetValue(end);
	valueInputField->OnInputAccepted([&](wiEventArgs args) {
		this->value = args.fValue;
		this->start = min(this->start, args.fValue);
		this->end = max(this->end, args.fValue);
		onSlide(args);
	});
	valueInputField->container = this;
}
wiSlider::~wiSlider()
{
	SAFE_DELETE(valueInputField);
}
void wiSlider::SetValue(float value)
{
	this->value = value;
}
float wiSlider::GetValue()
{
	return value;
}
void wiSlider::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
	{
		valueInputField->SetColor(this->colors[i], (WIDGETSTATE)i);
	}
	valueInputField->SetTextColor(this->textColor);
	valueInputField->SetTextShadowColor(this->textShadowColor);
	valueInputField->SetEnabled(IsEnabled());
	valueInputField->Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	bool dragged = false;

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE)
	{
		if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
		{
			if (state == ACTIVE)
			{
				// continue drag if already grabbed wheter it is intersecting or not
				dragged = true;
			}
		}
		else
		{
			gui->DeactivateWidget(this);
		}
	}

	float headWidth = hitBox.siz.x*0.05f;

	hitBox.pos.x -= headWidth*0.5f;
	hitBox.siz.x += headWidth;

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));


	if (pointerHitbox.intersects(hitBox))
	{
		// hover the slider
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}


	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		value = wiMath::InverseLerp(hitBox.pos.x, hitBox.pos.x + hitBox.siz.x, args.clickPos.x);
		value = wiMath::Clamp(value, 0, 1);
		value *= step;
		value = floorf(value);
		value /= step;
		value = wiMath::Lerp(start, end, value);
		args.fValue = value;
		args.iValue = (int)value;
		onSlide(args);
		gui->ActivateWidget(this);
	}

	valueInputField->SetValue(value);
}
void wiSlider::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	float headWidth = hitBox.siz.x*0.05f;

	gui->ResetScissor();

	// trail
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x - headWidth*0.5f, hitBox.pos.y + hitBox.siz.y * 0.5f - hitBox.siz.y*0.1f, hitBox.siz.x + headWidth, hitBox.siz.y * 0.2f), gui->GetGraphicsThread());
	// head
	float headPosX = wiMath::Lerp(hitBox.pos.x, hitBox.pos.x + hitBox.siz.x, wiMath::Clamp(wiMath::InverseLerp(start, end, value), 0, 1));
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(headPosX - headWidth * 0.5f, hitBox.pos.y, headWidth, hitBox.siz.y), gui->GetGraphicsThread());

	//if (parent != nullptr)
	//{
	//	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	//}
	// text
	wiFont(text, wiFontProps((int)(hitBox.pos.x - headWidth * 0.5f), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_RIGHT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor )).Draw(gui->GetGraphicsThread());

	//// value
	//stringstream ss("");
	//ss << value;
	//wiFont(ss.str(), wiFontProps((int)(hitBox.pos.x + hitBox.siz.x + headWidth), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_LEFT, WIFALIGN_CENTER, 2, 1,
	//	textColor, textShadowColor )).Draw(gui->GetGraphicsThread(), parent != nullptr);



	valueInputField->Render(gui);
}
void wiSlider::OnSlide(function<void(wiEventArgs args)> func)
{
	onSlide = move(func);
}





wiCheckBox::wiCheckBox(const std::string& name) :wiWidget()
	,checked(false)
{
	SetName(name);
	SetText(fastName.GetString());
	OnClick([](wiEventArgs args) {});
	SetSize(XMFLOAT2(20,20));
}
wiCheckBox::~wiCheckBox()
{

}
void wiCheckBox::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE)
	{
		gui->DeactivateWidget(this);
	}

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			clicked = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}

	if (clicked)
	{
		SetCheck(!GetCheck());
		wiEventArgs args;
		args.clickPos = pointerHitbox.pos;
		args.bValue = GetCheck();
		onClick(args);
		gui->ActivateWidget(this);
	}

}
void wiCheckBox::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	gui->ResetScissor();

	// control
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());

	// check
	if (GetCheck())
	{
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(wiColor::lerp(color, wiColor::White, 0.8f))
			, wiImageEffects(hitBox.pos.x + hitBox.siz.x*0.25f, hitBox.pos.y + hitBox.siz.y*0.25f, hitBox.siz.x*0.5f, hitBox.siz.y*0.5f)
			, gui->GetGraphicsThread());
	}

	//if (parent != nullptr)
	//{
	//	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	//}
	wiFont(text, wiFontProps((int)(hitBox.pos.x), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_RIGHT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor )).Draw(gui->GetGraphicsThread());

}
void wiCheckBox::OnClick(function<void(wiEventArgs args)> func)
{
	onClick = move(func);
}
void wiCheckBox::SetCheck(bool value)
{
	checked = value;
}
bool wiCheckBox::GetCheck()
{
	return checked;
}





wiComboBox::wiComboBox(const std::string& name) :wiWidget()
, selected(-1), combostate(COMBOSTATE_INACTIVE), hovered(-1)
{
	SetName(name);
	SetText(fastName.GetString());
	OnSelect([](wiEventArgs args) {});
	SetSize(XMFLOAT2(100, 20));
	SetMaxVisibleItemCount(8);
	firstItemVisible = 0;
}
wiComboBox::~wiComboBox()
{

}
const float wiComboBox::_GetItemOffset(int index) const
{
	index = max(firstItemVisible, index) - firstItemVisible;
	return hitBox.siz.y * (index + 1) + 1;
}
void wiComboBox::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}

	if (state == FOCUS)
	{
		state = IDLE;
	}
	if (state == DEACTIVATING)
	{
		state = IDLE;
	}
	if (state == ACTIVE && combostate == COMBOSTATE_SELECTING)
	{
		gui->DeactivateWidget(this);
	}

	hitBox.siz.x += hitBox.siz.y + 1; // + drop-down indicator arrow + little offset

	Hitbox2D pointerHitbox = Hitbox2D(gui->GetPointerPos(), XMFLOAT2(1, 1));

	bool clicked = false;
	// hover the button
	if (pointerHitbox.intersects(hitBox))
	{
		if (state == IDLE)
		{
			state = FOCUS;
		}
	}

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		// activate
		clicked = true;
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == DEACTIVATING)
		{
			// Keep pressed until mouse is released
			gui->ActivateWidget(this);
		}
	}


	if (clicked && state == FOCUS)
	{
		gui->ActivateWidget(this);
	}


	if (state == ACTIVE)
	{
		if (combostate == COMBOSTATE_INACTIVE)
		{
			combostate = COMBOSTATE_HOVER;
		}
		else if (combostate == COMBOSTATE_SELECTING)
		{
			gui->DeactivateWidget(this);
			combostate = COMBOSTATE_INACTIVE;
		}
		else
		{
			int scroll = (int)wiInputManager::GetInstance()->getpointer().z;
			firstItemVisible -= scroll;
			firstItemVisible = max(0, min((int)items.size() - maxVisibleItemCount, firstItemVisible));

			hovered = -1;
			for (size_t i = 0; i < items.size(); ++i)
			{
				if ((int)i<firstItemVisible || (int)i>firstItemVisible + maxVisibleItemCount)
				{
					continue;
				}

				Hitbox2D itembox;
				itembox.pos.y = hitBox.pos.y + _GetItemOffset((int)i);

				if (pointerHitbox.intersects(itembox))
				{
					hovered = (int)i;
					break;
				}
			}

			if (clicked)
			{
				combostate = COMBOSTATE_SELECTING;
				if (hovered >= 0)
				{
					SetSelected(hovered);
					gui->DeactivateWidget(this);
					combostate = COMBOSTATE_INACTIVE;
				}
			}
		}
	}

}
void wiComboBox::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();
	if (combostate != COMBOSTATE_INACTIVE)
	{
		color = colors[FOCUS];
	}

	gui->ResetScissor();

	// control-base
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());
	// control-arrow
	wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
		, wiImageEffects(hitBox.pos.x+hitBox.siz.x+1, hitBox.pos.y, hitBox.siz.y, hitBox.siz.y), gui->GetGraphicsThread());
	wiFont("V", wiFontProps((int)(hitBox.pos.x+hitBox.siz.x+hitBox.siz.y*0.5f), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread());


	//if (parent != nullptr)
	//{
	//	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	//}
	wiFont(text, wiFontProps((int)(hitBox.pos.x), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_RIGHT, WIFALIGN_CENTER, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread());

	if (selected >= 0)
	{
		wiFont(items[selected], wiFontProps((int)(hitBox.pos.x + hitBox.siz.x*0.5f), (int)(hitBox.pos.y + hitBox.siz.y*0.5f), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1,
			textColor, textShadowColor)).Draw(gui->GetGraphicsThread());
	}

	// drop-down
	if (state == ACTIVE)
	{
		gui->ResetScissor();

		// control-list
		int i = 0;
		for (auto& x : items)
		{
			if (i<firstItemVisible || i>firstItemVisible + maxVisibleItemCount)
			{
				i++;
				continue;
			}

			wiColor col = colors[IDLE];
			if (hovered == i)
			{
				if (combostate == COMBOSTATE_HOVER)
				{
					col = colors[FOCUS];
				}
				else if (combostate == COMBOSTATE_SELECTING)
				{
					col = colors[ACTIVE];
				}
			}
			wiImage::Draw(wiTextureHelper::getInstance()->getColor(col)
				, wiImageEffects(hitBox.pos.x, hitBox.pos.y + _GetItemOffset(i), hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());
			wiFont(x, wiFontProps((int)(hitBox.pos.x + hitBox.siz.x*0.5f), (int)(hitBox.pos.y + hitBox.siz.y*0.5f +_GetItemOffset(i)), -1, WIFALIGN_CENTER, WIFALIGN_CENTER, 2, 1,
				textColor, textShadowColor)).Draw(gui->GetGraphicsThread());
			i++;
		}
	}
}
void wiComboBox::OnSelect(function<void(wiEventArgs args)> func)
{
	onSelect = move(func);
}
void wiComboBox::AddItem(const std::string& item)
{
	items.push_back(item);

	if (selected < 0)
	{
		selected = 0;
	}
}
void wiComboBox::RemoveItem(int index)
{
	std::vector<string> newItems(0);
	newItems.reserve(items.size());
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (i != index)
		{
			newItems.push_back(items[i]);
		}
	}
	items = newItems;

	if (items.empty())
	{
		selected = -1;
	}
	else if (selected > index)
	{
		selected--;
	}
}
void wiComboBox::ClearItems()
{
	items.clear();

	selected = -1;
}
void wiComboBox::SetMaxVisibleItemCount(int value)
{
	maxVisibleItemCount = value;
}
void wiComboBox::SetSelected(int index)
{
	selected = index;

	wiEventArgs args;
	args.iValue = selected;
	args.sValue = GetItemText(selected);
	onSelect(args);
}
string wiComboBox::GetItemText(int index)
{
	if (index >= 0)
	{
		return items[index];
	}
	return "";
}
int wiComboBox::GetSelected()
{
	return selected;
}






static const float windowcontrolSize = 20.0f;
wiWindow::wiWindow(wiGUI* gui, const std::string& name) :wiWidget()
, gui(gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	SetName(name);
	SetText(fastName.GetString());
	SetSize(XMFLOAT2(640, 480));

	// Add controls

	SAFE_INIT(closeButton);
	SAFE_INIT(moveDragger);
	SAFE_INIT(resizeDragger_BottomRight);
	SAFE_INIT(resizeDragger_UpperLeft);



	// Add a grabber onto the title bar
	moveDragger = new wiButton(name + "_move_dragger");
	moveDragger->SetText("");
	moveDragger->SetSize(XMFLOAT2(hitBox.siz.x - windowcontrolSize * 3, windowcontrolSize));
	moveDragger->SetPos(XMFLOAT2(windowcontrolSize, 0));
	moveDragger->OnDrag([this](wiEventArgs args) {
		//this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
	});
	AddWidget(moveDragger);

	// Add close button to the top right corner
	closeButton = new wiButton(name + "_close_button");
	closeButton->SetText("x");
	closeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	closeButton->SetPos(XMFLOAT2(hitBox.pos.x + hitBox.siz.x - windowcontrolSize, hitBox.pos.y));
	closeButton->OnClick([this](wiEventArgs args) {
		this->SetVisible(false);
	});
	closeButton->SetTooltip("Close window");
	AddWidget(closeButton);

	// Add minimize button to the top right corner
	minimizeButton = new wiButton(name + "_minimize_button");
	minimizeButton->SetText("-");
	minimizeButton->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	minimizeButton->SetPos(XMFLOAT2(hitBox.pos.x + hitBox.siz.x - windowcontrolSize *2, hitBox.pos.y));
	minimizeButton->OnClick([this](wiEventArgs args) {
		this->SetMinimized(!this->IsMinimized());
	});
	minimizeButton->SetTooltip("Minimize window");
	AddWidget(minimizeButton);

	// Add a resizer control to the upperleft corner
	resizeDragger_UpperLeft = new wiButton(name + "_resize_dragger_upper_left");
	resizeDragger_UpperLeft->SetText("");
	resizeDragger_UpperLeft->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	resizeDragger_UpperLeft->SetPos(XMFLOAT2(0, 0));
	resizeDragger_UpperLeft->OnDrag([this](wiEventArgs args) {
		XMFLOAT2 scaleDiff;
		scaleDiff.x = (hitBox.siz.x - args.deltaPos.x) / hitBox.siz.x;
		scaleDiff.y = (hitBox.siz.y - args.deltaPos.y) / hitBox.siz.y;
		//this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
		//this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
	});
	AddWidget(resizeDragger_UpperLeft);

	// Add a resizer control to the bottom right corner
	resizeDragger_BottomRight = new wiButton(name + "_resize_dragger_bottom_right");
	resizeDragger_BottomRight->SetText("");
	resizeDragger_BottomRight->SetSize(XMFLOAT2(windowcontrolSize, windowcontrolSize));
	resizeDragger_BottomRight->SetPos(XMFLOAT2(hitBox.pos.x + hitBox.siz.x - windowcontrolSize, hitBox.pos.y + hitBox.siz.y - windowcontrolSize));
	resizeDragger_BottomRight->OnDrag([this](wiEventArgs args) {
		XMFLOAT2 scaleDiff;
		scaleDiff.x = (hitBox.siz.x + args.deltaPos.x) / hitBox.siz.x;
		scaleDiff.y = (hitBox.siz.y + args.deltaPos.y) / hitBox.siz.y;
		//this->Scale(XMFLOAT3(scaleDiff.x, scaleDiff.y, 1));
	});
	AddWidget(resizeDragger_BottomRight);


	SetEnabled(true);
	SetVisible(true);
	SetMinimized(false);
}
wiWindow::~wiWindow()
{
	RemoveWidgets(true);
}
void wiWindow::AddWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	widget->SetEnabled(this->IsEnabled());
	widget->SetVisible(this->IsVisible());
	gui->AddWidget(widget);
	//widget->attachTo(this);
	widget->container = this;

	childrenWidgets.push_back(widget);
}
void wiWindow::RemoveWidget(wiWidget* widget)
{
	assert(gui != nullptr && "Ivalid GUI!");

	gui->RemoveWidget(widget);
	//widget->detach();

	childrenWidgets.remove(widget);
}
void wiWindow::RemoveWidgets(bool alsoDelete)
{
	assert(gui != nullptr && "Ivalid GUI!");

	for (auto& x : childrenWidgets)
	{
		//x->detach();
		gui->RemoveWidget(x);
		if (alsoDelete)
		{
			SAFE_DELETE(x);
		}
	}

	childrenWidgets.clear();
}
void wiWindow::Update(wiGUI* gui, float dt )
{
	wiWidget::Update(gui, dt);


	if (!IsEnabled())
	{
		return;
	}

	for (auto& x : childrenWidgets)
	{
		x->Update(gui, dt);
		x->SetScissorRect(scissorRect);
	}

	if (gui->IsWidgetDisabled(this))
	{
		return;
	}
}
void wiWindow::Render(wiGUI* gui)
{
	assert(gui != nullptr && "Ivalid GUI!");

	if (!IsVisible())
	{
		return;
	}

	wiColor color = GetColor();

	gui->ResetScissor();

	// body
	if (!IsMinimized())
	{
		wiImage::Draw(wiTextureHelper::getInstance()->getColor(color)
			, wiImageEffects(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y), gui->GetGraphicsThread());
	}

	for (auto& x : childrenWidgets)
	{
		if (x != gui->GetActiveWidget())
		{
			// the gui will render the active on on top of everything!
			x->Render(gui);
		}
	}

	scissorRect.bottom = (LONG)(hitBox.pos.y + hitBox.siz.y);
	scissorRect.left = (LONG)(hitBox.pos.x);
	scissorRect.right = (LONG)(hitBox.pos.x + hitBox.siz.x);
	scissorRect.top = (LONG)(hitBox.pos.y);
	wiRenderer::GetDevice()->BindScissorRects(1, &scissorRect, gui->GetGraphicsThread());
	wiFont(text, wiFontProps((int)(hitBox.pos.x + resizeDragger_UpperLeft->hitBox.siz.x + 2), (int)(hitBox.pos.y), -1, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1,
		textColor, textShadowColor)).Draw(gui->GetGraphicsThread());

}
void wiWindow::SetVisible(bool value)
{
	wiWidget::SetVisible(value);
	SetMinimized(!value);
	for (auto& x : childrenWidgets)
	{
		x->SetVisible(value);
	}
}
void wiWindow::SetEnabled(bool value)
{
	//wiWidget::SetEnabled(value);
	for (auto& x : childrenWidgets)
	{
		if (x == moveDragger)
			continue;
		if (x == minimizeButton)
			continue;
		if (x == closeButton)
			continue;
		if (x == resizeDragger_UpperLeft)
			continue;
		x->SetEnabled(value);
	}
}
void wiWindow::SetMinimized(bool value)
{
	minimized = value;

	if (resizeDragger_BottomRight != nullptr)
	{
		resizeDragger_BottomRight->SetVisible(!value);
	}
	for (auto& x : childrenWidgets)
	{
		if (x == moveDragger)
			continue;
		if (x == minimizeButton)
			continue;
		if (x == closeButton)
			continue;
		if (x == resizeDragger_UpperLeft)
			continue;
		x->SetVisible(!value);
	}
}
bool wiWindow::IsMinimized()
{
	return minimized;
}




wiColorPicker::wiColorPicker(wiGUI* gui, const std::string& name) :wiWindow(gui, name)
{
	SetSize(XMFLOAT2(300, 260));
	SetColor(wiColor::Ghost);
	RemoveWidget(resizeDragger_BottomRight);
	RemoveWidget(resizeDragger_UpperLeft);

	hue_picker = XMFLOAT2(0, 0);
	saturation_picker = XMFLOAT2(0, 0);
	saturation_picker_barycentric = XMFLOAT3(0, 1, 0);
	hue_color = XMFLOAT4(1, 0, 0, 1);
	final_color = XMFLOAT4(1, 1, 1, 1);
	angle = 0;
}
wiColorPicker::~wiColorPicker()
{
}
static const float __colorpicker_center = 120;
static const float __colorpicker_radius_triangle = 75;
static const float __colorpicker_radius = 80;
static const float __colorpicker_width = 16;
void wiColorPicker::Update(wiGUI* gui, float dt )
{
	wiWindow::Update(gui, dt);

	if (!IsEnabled())
	{
		return;
	}

	//if (!gui->IsWidgetDisabled(this))
	//{
	//	return;
	//}

	if (state == DEACTIVATING)
	{
		state = IDLE;
	}

	XMFLOAT2 center = XMFLOAT2(hitBox.pos.x + __colorpicker_center, hitBox.pos.y + __colorpicker_center);
	XMFLOAT2 pointer = gui->GetPointerPos();
	float distance = wiMath::Distance(center, pointer);
	bool hover_hue = (distance > __colorpicker_radius) && (distance < __colorpicker_radius + __colorpicker_width);

	float distTri = 0;
	XMFLOAT4 A, B, C;
	wiMath::ConstructTriangleEquilateral(__colorpicker_radius_triangle, A, B, C);
	XMVECTOR _A = XMLoadFloat4(&A);
	XMVECTOR _B = XMLoadFloat4(&B);
	XMVECTOR _C = XMLoadFloat4(&C);
	XMMATRIX _triTransform = XMMatrixRotationZ(-angle) * XMMatrixTranslation(center.x, center.y, 0);
	_A = XMVector4Transform(_A, _triTransform);
	_B = XMVector4Transform(_B, _triTransform);
	_C = XMVector4Transform(_C, _triTransform);
	XMVECTOR O = XMVectorSet(pointer.x, pointer.y, 0, 0);
	XMVECTOR D = XMVectorSet(0, 0, 1, 0);
	bool hover_saturation = TriangleTests::Intersects(O, D, _A, _B, _C, distTri);

	if (hover_hue && state==IDLE)
	{
		state = FOCUS;
		huefocus = true;
	}
	else if (hover_saturation && state == IDLE)
	{
		state = FOCUS;
		huefocus = false;
	}
	else if (state == IDLE)
	{
		huefocus = false;
	}

	bool dragged = false;

	if (wiInputManager::GetInstance()->press(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == FOCUS)
		{
			// activate
			dragged = true;
		}
	}

	if (wiInputManager::GetInstance()->down(VK_LBUTTON, wiInputManager::KEYBOARD))
	{
		if (state == ACTIVE)
		{
			// continue drag if already grabbed wheter it is intersecting or not
			dragged = true;
		}
	}

	dragged = dragged && !gui->IsWidgetDisabled(this);
	if (huefocus && dragged)
	{
		//hue pick
		angle = wiMath::GetAngle(XMFLOAT2(pointer.x - center.x, pointer.y - center.y), XMFLOAT2(__colorpicker_radius, 0));
		XMFLOAT3 color3 = wiMath::HueToRGB(angle / XM_2PI);
		hue_color = XMFLOAT4(color3.x, color3.y, color3.z, 1);
		gui->ActivateWidget(this);
	}
	else if (!huefocus && dragged)
	{
		// saturation pick
		wiMath::GetBarycentric(O, _A, _B, _C, saturation_picker_barycentric.x, saturation_picker_barycentric.y, saturation_picker_barycentric.z, true);
		gui->ActivateWidget(this);
	}
	else if(state != IDLE)
	{
		gui->DeactivateWidget(this);
	}

	float r = __colorpicker_radius + __colorpicker_width*0.5f;
	hue_picker = XMFLOAT2(center.x + r*cos(angle), center.y + r*-sin(angle));
	XMStoreFloat2(&saturation_picker, _A*saturation_picker_barycentric.x + _B*saturation_picker_barycentric.y + _C*saturation_picker_barycentric.z);
	XMStoreFloat4(&final_color, XMLoadFloat4(&hue_color)*saturation_picker_barycentric.x + XMVectorSet(1, 1, 1, 1)*saturation_picker_barycentric.y + XMVectorSet(0, 0, 0, 1)*saturation_picker_barycentric.z);

	if (dragged)
	{
		wiEventArgs args;
		args.clickPos = pointer;
		args.fValue = angle;
		args.color = final_color;
		onColorChanged(args);
	}
}
void wiColorPicker::Render(wiGUI* gui)
{
	wiWindow::Render(gui);


	if (!IsVisible() || IsMinimized())
	{
		return;
	}
	GRAPHICSTHREAD threadID = gui->GetGraphicsThread();

	struct Vertex
	{
		XMFLOAT4 pos;
		XMFLOAT4 col;
	};
	static wiGraphicsTypes::GPUBuffer vb_saturation;
	static wiGraphicsTypes::GPUBuffer vb_hue;
	static wiGraphicsTypes::GPUBuffer vb_picker;
	static wiGraphicsTypes::GPUBuffer vb_preview;

	static std::vector<Vertex> vertices_saturation(0);

	static bool buffersComplete = false;
	if (!buffersComplete)
	{
		buffersComplete = true;

		HRESULT hr = S_OK;
		// saturation
		{
			vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,0,0,1) });	// hue
			vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(1,1,1,1) });	// white
			vertices_saturation.push_back({ XMFLOAT4(0,0,0,0),XMFLOAT4(0,0,0,1) });	// black
			wiMath::ConstructTriangleEquilateral(__colorpicker_radius_triangle, vertices_saturation[0].pos, vertices_saturation[1].pos, vertices_saturation[2].pos);

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices_saturation.size() * sizeof(Vertex));
			desc.CPUAccessFlags = CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_DYNAMIC;
			SubresourceData data;
			data.pSysMem = vertices_saturation.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_saturation);
		}
		// hue
		{
			std::vector<Vertex> vertices(0);
			for (float i = 0; i <= 100; i += 1.0f)
			{
				float p = i / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				XMFLOAT3 rgb = wiMath::HueToRGB(p);
				vertices.push_back({ XMFLOAT4(__colorpicker_radius * x, __colorpicker_radius * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
				vertices.push_back({ XMFLOAT4((__colorpicker_radius + __colorpicker_width) * x, (__colorpicker_radius + __colorpicker_width) * y, 0, 1), XMFLOAT4(rgb.x,rgb.y,rgb.z,1) });
			}

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_DYNAMIC;
			SubresourceData data;
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_hue);
		}
		// picker
		{
			float _radius = 3;
			float _width = 3;
			std::vector<Vertex> vertices(0);
			for (float i = 0; i <= 100; i += 1.0f)
			{
				float p = i / 100;
				float t = p * XM_2PI;
				float x = cos(t);
				float y = -sin(t);
				vertices.push_back({ XMFLOAT4(_radius * x, _radius * y, 0, 1), XMFLOAT4(1,1,1,1) });
				vertices.push_back({ XMFLOAT4((_radius + _width) * x, (_radius + _width) * y, 0, 1), XMFLOAT4(1,1,1,1) });
			}

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_picker);
		}
		// preview
		{
			float _width = 20;

			std::vector<Vertex> vertices(0);
			vertices.push_back({ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(-_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(-_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, _width, 0, 1),XMFLOAT4(1,1,1,1) });
			vertices.push_back({ XMFLOAT4(_width, -_width, 0, 1),XMFLOAT4(1,1,1,1) });

			GPUBufferDesc desc;
			desc.BindFlags = BIND_VERTEX_BUFFER;
			desc.ByteWidth = (UINT)(vertices.size() * sizeof(Vertex));
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			desc.Usage = USAGE_IMMUTABLE;
			SubresourceData data;
			data.pSysMem = vertices.data();
			hr = wiRenderer::GetDevice()->CreateBuffer(&desc, &data, &vb_preview);
		}


	}

	XMMATRIX __cam = wiRenderer::GetDevice()->GetScreenProjection();

	wiRenderer::GetDevice()->BindConstantBuffer(VS, wiRenderer::constantBuffers[CBTYPE_MISC], CBSLOT_RENDERER_MISC, threadID);
	wiRenderer::GetDevice()->BindGraphicsPSO(PSO_colorpicker, threadID);

	wiRenderer::MiscCB cb;

	// render saturation triangle
	{
		if (vb_saturation.IsValid() && !vertices_saturation.empty())
		{
			vertices_saturation[0].col = hue_color;
			wiRenderer::GetDevice()->UpdateBuffer(&vb_saturation, vertices_saturation.data(), threadID, vb_saturation.GetDesc().ByteWidth);
		}

		cb.mTransform = XMMatrixTranspose(
			XMMatrixRotationZ(-angle) *
			XMMatrixTranslation(hitBox.pos.x + __colorpicker_center, hitBox.pos.y + __colorpicker_center, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1, 1, 1, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		GPUBuffer* vbs[] = {
			&vb_saturation,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_saturation.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render hue circle
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(hitBox.pos.x + __colorpicker_center, hitBox.pos.y + __colorpicker_center, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1, 1, 1, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		GPUBuffer* vbs[] = {
			&vb_hue,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_hue.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render hue picker
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(hue_picker.x, hue_picker.y, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1 - hue_color.x, 1 - hue_color.y, 1 - hue_color.z, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		GPUBuffer* vbs[] = {
			&vb_picker,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_picker.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render saturation picker
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(saturation_picker.x, saturation_picker.y, 0) *
			__cam
		);
		cb.mColor = XMFLOAT4(1 - final_color.x, 1 - final_color.y, 1 - final_color.z, 1);
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		GPUBuffer* vbs[] = {
			&vb_picker,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_picker.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// render preview
	{
		cb.mTransform = XMMatrixTranspose(
			XMMatrixTranslation(hitBox.pos.x + 260, hitBox.pos.y + 40, 0) *
			__cam
		);
		cb.mColor = GetPickColor();
		wiRenderer::GetDevice()->UpdateBuffer(wiRenderer::constantBuffers[CBTYPE_MISC], &cb, threadID);
		GPUBuffer* vbs[] = {
			&vb_preview,
		};
		const UINT strides[] = {
			sizeof(Vertex),
		};
		wiRenderer::GetDevice()->BindVertexBuffers(vbs, 0, ARRAYSIZE(vbs), strides, nullptr, threadID);
		wiRenderer::GetDevice()->Draw(vb_preview.GetDesc().ByteWidth / sizeof(Vertex), 0, threadID);
	}

	// RGB values:
	stringstream _rgb("");
	_rgb << "R: " << (int)(final_color.x * 255) << endl;
	_rgb << "G: " << (int)(final_color.y * 255) << endl;
	_rgb << "B: " << (int)(final_color.z * 255) << endl;
	wiFont(_rgb.str(), wiFontProps((int)(hitBox.pos.x + 200), (int)(hitBox.pos.y + 200), -1, WIFALIGN_LEFT, WIFALIGN_TOP, 2, 1,
		textColor, textShadowColor)).Draw(threadID);
	
}
XMFLOAT4 wiColorPicker::GetPickColor()
{
	return final_color;
}
void wiColorPicker::SetPickColor(const XMFLOAT4& value)
{
	// TODO
}
void wiColorPicker::OnColorChanged(function<void(wiEventArgs args)> func)
{
	onColorChanged = move(func);
}
