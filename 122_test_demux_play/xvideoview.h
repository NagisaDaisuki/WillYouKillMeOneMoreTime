#ifndef XVIDEO_VIEW_H
#define XVIDEO_VIEW_H
#include <mutex>
#include <fstream>

// 前向声明
struct AVFrame;

// 自定义睡眠函数
void MSleep(unsigned int ms);

// 获取当前时间函数
long long NowMs();

//////////////////////////////////
//// 视频渲染接口类
//// 隐藏SDL实现
//// 渲染方案可替代
//// 线程安全
//////////////////////////////////
class XVideoView
{
public:
	enum PixFormat // 枚举值和 ffmpeg 中的值一致
	{
		YUV420P = 0,
		RGBA = 26,
		ARGB = 25,
		BGRA = 28,
		RGB24 = 2,
		NV12 = 23,
	};
	enum RenderType
	{
		SDL = 0,
		OPENGL,
		DIRECTX
	};

	virtual ~XVideoView();	// 虚析构函数

	// 静态成员函数创建类对象
	static XVideoView* Create(RenderType type = SDL);
	/////////////////////////////////////////
	//// 初始化渲染窗口 线程安全
	//// @para w 窗口宽度
	//// @para h 窗口高度
	//// @para fmt 像素格式
	//// @return 是否创建成功
	/////////////////////////////////////////
	virtual bool Init(int w, int h, 
		PixFormat fmt = RGBA) = 0;
	
	/////////////////////////////////////////
	//// 渲染图像 线程安全
	//// @para data 渲染的二进制图像数据
	//// @para linesize 图像每行字节数, 对于 YUV420P就是Y一行字节数
	//// linesize <= 0 就根据宽度和像素格式自动算出大小
	//// @return 是否渲染成功
	/////////////////////////////////////////
	virtual bool Draw(const unsigned char* data, int linesize = 0) = 0;
	virtual bool Draw(const unsigned char* y, int y_pitch,
						const unsigned char* u, int u_pitch,
						const unsigned char* v, int v_pitch) = 0;

	
	bool DrawFrame(AVFrame* frame);

	// 清理所有申请的资源，包括关闭窗口
	virtual void Close() = 0;

	// 处理窗口退出事件 
	virtual bool IsExit() = 0;

	// 窗口显示缩放
	void Scale(int w, int h) {
		scale_w_ = w;
		scale_h_ = h;
	}
		
	// 访问 fps 值
	int render_fps() { return render_fps_; }

	// 打开文件操作
	bool Open(std::string filepath);

	/////////////////////////////////////////
	//// 读取一帧数据，并维护 AVFrame 空间
	//// 每次调用会覆盖上一次数据
	/////////////////////////////////////////
	AVFrame* Read();

	// 设置窗口句柄
	void set_win_id(void* win) { win_id_ = win; }
	


protected: // 在派生类中访问
	void* win_id_ = nullptr; // 窗口句柄
	int render_fps_ = 0; // 显示帧率
	int width_ = 0; // 材质宽度
	int height_ = 0; // 材质高度
	PixFormat fmt_ = RGBA; // 像素格式
	std::mutex mtx_; // 线程安全互斥锁
	
	// 实际显示大小(窗口大小)
	int scale_w_ = 0;
	int scale_h_ = 0; 

	long long beg_ms_ = 0;		// 计时开始时间
	int count_ = 0;				// 统计显式次数
private:
	std::ifstream ifs_;
	AVFrame* frame_ = nullptr;
	unsigned char* cache_ = nullptr;	// 复制 NV12缓冲
};

#endif

