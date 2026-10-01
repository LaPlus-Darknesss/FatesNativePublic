#pragma once
class PostEffect { public: static void Initialize(); static void Finalize(); static void* Buffer(); private: static void* buffer_; };
