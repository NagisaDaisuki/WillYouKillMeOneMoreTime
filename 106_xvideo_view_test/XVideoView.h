#ifndef XVIDEO_VIEW_H
#define XVIDEO_VIEW_H
#include <mutex>

//////////////////////////////////
//// 视频渲染接口类
//// 隐藏SDL实现
//// 渲染方案可替代
//// 线程安全
//////////////////////////////////
class XVideoView
{
public:
	enum PixFormat
	{
		RGBA = 0,
		ARGB,
		YUV420P
	};
	enum RenderType
	{
		SDL = 0,
		OPENGL,
		DIRECTX
	};
	// 静态成员函数创建类对象
	static XVideoView* Create(RenderType type = SDL);
	/////////////////////////////////////////
	//// 初始化渲染窗口 线程安全
	//// @para w 窗口宽度
	//// @para h 窗口高度
	//// @para fmt 像素格式
	//// @para win_id 窗口句柄, 如果为空，创建新窗口
	//// @return 是否创建成功
	/////////////////////////////////////////
	virtual bool Init(int w, int h, 
		PixFormat fmt = RGBA, void* win_id = nullptr) = 0;
	
	/////////////////////////////////////////
	//// 渲染图像 线程安全
	//// @para data 渲染的二进制图像数据
	//// @para linesize 图像每行字节数, 对于 YUV420P就是Y一行字节数
	//// linesize <= 0 就根据宽度和像素格式自动算出大小
	//// @return 是否渲染成功
	/////////////////////////////////////////
	virtual bool Draw(const unsigned char* data, int linesize = 0) = 0;
	// 窗口显示缩放
	void Scale(int w, int h) {
		scale_w_ = w;
		scale_h_ = h;
	}
protected: // 在派生类中访问
	int width_ = 0; // 材质宽度
	int height_ = 0; // 材质高度
	PixFormat fmt_ = RGBA; // 像素格式
	std::mutex mtx_; // 线程安全互斥锁
	
	// 实际显示大小(窗口大小)
	int scale_w_ = 0;
	int scale_h_ = 0; 
};

#endif

