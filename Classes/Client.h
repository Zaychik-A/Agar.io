#pragma once

#include "cocos2d.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include "chat_message.hpp"
#include <vector>
#include <string>


/**
* @name ѯ���������Ƿ�����Ѿ������õ���Ϸ����
* @{
*/
#define QUERY_FOR_ROOM  "a"
/** @} */

/**
* @name ��ӦQUERY_FOR_ROOM, ���ṩ���������Ϣ
* @{
*/
#define ANSWER_FOR_ROOM "b"
/** @} */

/**
* @name ������䱻����
* @{
*/
#define START_FAILED       "c"
/** @} */

/**
* @name �㲥��ʼ��Ϸָ��
* @{
*/
#define START_GAME      "d"
/** @} */

/**
* @name �򷿼���������Ҵ����˳�����������Ϣ
* @{
*/
#define EXIT_ROOM       "e"
/** @} */

/**
* @name �¼��뷿�������򷿼�ӵ�������󷿼���������ҵ���Ϣ
* @{
*/
#define QUERY_FOR_PLAYERS_IN_ROOM "f"
/** @} */

/**
* @name QUERY_FOR_PLAYERS_IN_ROOM, ����������\n
* 		���¼����ߴ��䷿����������ҵ���Ϣ
* @{
*/
#define ANSWER_FOR_PLAYERS_IN_ROOM "g"
/** @} */

/**
* @name �򷿼���������Ҵ���������Ϣ
* @{
*/
#define CHAT_MESSAGE    "h"
/** @} */

/**
* @name ����λ��
* @{
*/
#define INIT_POS      "i"
/** @} */

/**
* @name ����Ϸ��������ҹ㲥������Ϣ��ȷ��ͬ������
* @{
*/
//example j12|||||||||||||||
//j for DEAD_MESSAGE
//1 for player1
//2 for player1's ball 2(start from 0)
#define DEAD_MESSAGE    "j"
/** @} */

//ѯ�ʿ�ʼ
#define QUERY_FOR_START "k"

//��ӦQUERY_FOR_START
#define ANSWER_FOR_START "l"

//��������
//began: 1, moved: 2, ended: 3
//
//tan: m1 > 77.5 > m2 > 22.5  > m3
//
//		+y		-y
//+x	1		2
//-x	3		4
//
//example: m2134|||||||||||
//m for MOVE_ACTION
//2 for player2
//1 for touch began
//3 for tan < 22.5
//4 for x < 0, y < 0
#define MOVE_ACTION "m"

//����
//example n8120||||||||||||
//n for DIVIDE_ACTION
//8 for player8
//1 for move1
//2 for x > 0, y < 0
//0 for if_is_moving_ol == false
#define DIVIDE_ACTION "n"

//���
//example: o12345|||||||||||
//o for PLAYER_SCALE
//1 for player1
//2 for player1's ball 2 (start from 0)
//345 for scale 3.45
#define PLAYER_SCALE "o"

//λ��
//example: p219876541234561|
//p for PLAYER_POSITION
//2 for player2
//1 for player2's ball 1 (start from 0)
//987654 for x 9876.54
//123456 for y 1234.56
//1 for x > 0, y > 0
//		+y		-y
//+x	1		2
//-x	3		4
#define PLAYER_POSITION "p"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

/**
*   @brief  ���ݽ��ա�������\n
*           һ�δ洢������һ������, �첽���ա���������\n
*/
class chat_client
{
public:

	/**
	* @brief                        ����һ���������ӵĿͻ���
	*
	* @param io_service             asio�ĺ�����, ���ڴ���socket
	* @param endpoint_iterator      Ҫ����λ�õĶ˿ں�
	*
	*/
	chat_client(boost::asio::io_service& io_service,
		tcp::resolver::iterator endpoint_iterator)
		:
		io_service_(io_service), socket_(io_service)
	{
		boost::asio::async_connect(socket_, endpoint_iterator,
			boost::bind(&chat_client::handle_connect, this,
				boost::asio::placeholders::error)); //���еĲ����������첽�ķ�ʽ
	}

	/**
	* @brief        ��ָ���io_service�Է��͸�server
	*
	* @param msg    �Զ����ʽ�����ݰ�
	*
	*/
	void write(const chat_message& msg)
	{
		io_service_.post(boost::bind(&chat_client::do_write, this, msg));
	}

	/**
	* @brief        �ͻ���������ֹʱ���øú���
	*/
	void close()
	{
		io_service_.post(boost::bind(&chat_client::do_close, this));
	}

private:

	/**
	* @brief        �ɹ���������ʱ���øú���
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_connect(const boost::system::error_code& error);

	/**
	* @brief        �ڽ��ܵ���Ϣ���������ַ����������ô�������ĺ���(handle_read_body)
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_read_header(const boost::system::error_code& error);

	/**
	* @brief        �����������ݲ��������ַ������Ⱥ���øú��������������, �ڲ���\n
	*               �����������ȡ��ͷ, �ڶ�ȡ���������handle_read_header��������
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_read_body(const boost::system::error_code& error);
		
	/**
	* @brief        ��Ҫ���͵�����ѹ��ȴ����͵����ݶ��У��ڲ���ѹ������������ȷ���һ\n
	*               �����ݰ�����������͵����ݰ�����һ���Ļ������handle_connect����\n
	*               ʣ������ݰ�
	*
	* @param msg    Ҫ���͵��Ѿ�����õ����ݰ�
	*
	*/
	void do_write(chat_message msg);

	/**
	* @brief        �ݹ鷢�͵ȴ����͵����ݣ���û�����ݷ���ʱֹͣ�ݹ�
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_write(const boost::system::error_code& error);

	/**
	* @brief        �ӷ�������ȡ��Ϣʧ��ʱ����
	*/
	void do_close()
	{
		socket_.close();
	}

private:
	boost::asio::io_service&    io_service_;    ///asio�ĺ�����, ���ڴ���socket
	tcp::socket                 socket_;        ///tcp���׽���
	chat_message                read_msg_;      ///ÿ��ѭ������ȡ�����ݶ���
	chat_message_queue          write_msgs_;    ///�ȴ������͵����ݶ���
};

//----------------------------------------------------------------------

/**
*   @brief  ����������server\n
*           �����ֹ���ģʽ���ھ������¸������������Ӿ�������\n
*           ��server���ڻ�����ģʽ����ֱ������Զ��server
*/
class Client : public cocos2d::Node
{
public:

	bool                        _search_finished;       ///�Ƿ������������server����������        
	bool                        _filter_mode;           ///�Ƿ���filter mode
	bool                        _with_server;           ///client����ʱ�Ƿ���serverͬ������
	chat_client*                _clientInstance;        ///���ڴ������ݵĽ��ܺͷ���
	std::mutex                  t_lock;                 ///�߳������������߳��¶����ݽ���ͬʱ����
	std::string                 sensitive_word;         ///filter mode���������ָ������д�
	std::deque<std::string>     _orderList;             ///��ȡ������ȷָ��

														/**
														* @brief    ����client��������
														*/
	static Client* create(int mode)
	{
		Client *sprite = new Client();
		if (sprite)
		{
			//remember to release?
			//remember to release?
			//remember to release?

			sprite->autorelease();
			sprite->runClient(mode);

			return sprite;
		}
		CC_SAFE_DELETE(sprite);
		return nullptr;
	}

	/**
	* @brief    ����client
	*
	* @param    mode         ����client��ģʽ
	*        -<em>1</em>    ͬʱ��server����
	*        -<em>2/em>     ֻ����client
	*
	*/
	void runClient(int mode);

	/**
	* @brief    ȡ����ȡ����ָ��
	*
	* @return   ��ȡ�����������˵�ָ��
	*        -<em>"no"</em>             û�и���ָ��
	*        -<em>std::string</em>      ��ȷָ��
	*/
	std::string executeOrder(void);

	/**
	* @brief    ����client
	*
	* @param    code        ָ����
	* @param    message     Ҫ���͵�ָ��
	*
	*/
	void sendMessage(const std::string & code, const std::string & message);

	/**
	* @brief    ����client������������server
	*
	* @return   �Ƿ���������
	*/
	int client(void);

	void close()
	{
		_clientInstance->close();
	}
};

//extern Client* this_client;
extern int player_num;
extern int player_count;