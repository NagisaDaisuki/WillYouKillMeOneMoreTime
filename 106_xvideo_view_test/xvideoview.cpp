#include "stdafx.h"
#include "xvideoview.h"
#include "xsdl.h"

XVideoView* XVideoView::Create(RenderType type) {
	switch (type)
	{
	case XVideoView::SDL:
		return new XSDL();
		break;
	default:
		break;
	}

	return nullptr;
}
