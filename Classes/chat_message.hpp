#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
* @brief server��client���õ����ݰ���\n
* һ�����ݷ�Ϊ�����֣�\n
* 1���ַ���ͷ��4���ֽڱ�ʾ���ݲ��ֵĳ��ȣ����Ϊ9999�����㲹0\n
* 2���ַ���������ַ���ʾ�������ͣ����Client.h\n
* 3���ַ�������Ϊ��������
* 4���磺һ����Ϣ\t019b2player2|styisagay
* \t\t\t���Ϊ�������19���ַ�, Ϊһ��ANSWER_FOR_ROOM����, ��ʾ\n
* \t\t\t���Ƿ����ڵĵڶ������, ��ɫΪplayer2, �������Ϊstyisagay
*/

class chat_message
{
public:
	enum { header_length = 4 };
	enum { max_body_length = 512 };

	chat_message()
		: body_length_(0)
	{
	}

	const char* data() const
	{
		return data_;
	}

	char* data()
	{
		return data_;
	}

	size_t length() const
	{
		return header_length + body_length_;
	}

	const char* body() const
	{
		return data_ + header_length;
	}

	char* body()
	{
		return data_ + header_length;
	}

	size_t body_length() const
	{
		return body_length_;
	}

	void body_length(size_t new_length)
	{
		body_length_ = new_length;
		if (body_length_ > max_body_length)
			body_length_ = max_body_length;
	}

	/**
	* @brief ����ͷ��4�ֽ��ַ���ת��������
	*
	* @return ����˵��
	*        -<em>false</em> ת����������Ƿ�
	*        -<em>true</em> �ɹ����룬chat_messageʵ���ڵ�body_length�Ѿ��ɹ�����
	*/

	bool decode_header()
	{
		using namespace std;
		char header[header_length + 1] = "";
		strncat(header, data_, header_length);
		body_length_ = atoi(header);
		if (body_length_ > max_body_length)
		{
			body_length_ = 0;
			return false;
		}
		return true;
	}

	/**
	* @brief ���ַ������ȱ��뵽����ͷ��
	*/

	void encode_header()
	{
		using namespace std;
		char header[header_length + 1] = "";
		sprintf(header, "%4d", body_length_);
		memcpy(data_, header, header_length);
	}

private:
	char data_[header_length + max_body_length];
	size_t body_length_;
}; 

#endif // CHAT_MESSAGE_HPP