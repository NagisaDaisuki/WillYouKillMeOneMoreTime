#pragma once
#include <mutex>
#include <vector>
// 前置声明
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
struct AVPacket;
void PrintErr(int err);

////////////////////////////////////
/// 编码和解码的基类(非纯抽象类)
////////////////////////////////////
class XCodec
{
public:
	~XCodec();

	///////////////////////////////////////////////
	/// 创建编码上下文 
	/// 与成员无关 static
	/// @para codec_id 编码器 ID号，对应 FFmpeg
	/// @return 编码上下文，失败返回 nullptr
	///////////////////////////////////////////////
	static AVCodecContext* Create(int codec_id,bool is_encode);
	
	///////////////////////////////////////////////
	/// 设置对象的编码器上下文 上下文传递到对象中，空间由 XEncode维护
	/// 加锁，线程安全 （在调用 set_c 时可能有的程序在使用 c_）
	///	@para c 编码器上下文 如果 c_ 不为 nullptr，则先清理资源
	///////////////////////////////////////////////
	void set_c(AVCodecContext* c);


	///////////////////////////////////////////////
	/// 设置编码参数，线程安全
	/// C++ 支持函数重载
	///////////////////////////////////////////////
	bool SetOpt(const char* key, const char* value);
	bool SetOpt(const char* key,int value);


	///////////////////////////////////////////////
	/// 打开编码器 线程安全
	///////////////////////////////////////////////
	bool Open();

	///////////////////////////////////////////////
	/// 创建一个 AVFrame，根据 AVCodecContext，需要调用
	/// 者释放
	///////////////////////////////////////////////
	AVFrame* CreateFrame();

protected: // 基类成员做protected方便子类成员调用
	AVCodecContext* c_ = nullptr;	// 编码器上下文
	std::mutex mtx_;
};

