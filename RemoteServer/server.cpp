#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include "chat_message.hpp"

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

//==================================chart_participant========================

/**
*   @brief  chart_session�Ļ���\n
*           ��֤ÿ��chart_session����Ҫ�����Լ���deliver����
*/

class chat_participant
{
public:
	virtual ~chat_participant() {}
	virtual void deliver(const chat_message& msg) = 0;
};

typedef boost::shared_ptr<chat_participant> chat_participant_ptr;

//==================================chat_room===============================

/**
*   @brief  chart_session�Ļ���\n
*           ��֤ÿ��chart_session����Ҫ�����Լ���deliver����
*/

class chat_room
{
public:

	/**
	* @brief                ���¼���ĻỰ���뷿�����Ա��Թ㲥�ķ�ʽ��������
	*
	* @param participant    �ѳɹ����ӵġ���Ҫ�������췿��ĻỰ
	*
	*/
	void join(chat_participant_ptr participant)
	{
		participants_.insert(participant);
		std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
			boost::bind(&chat_participant::deliver, participant, _1));
	}

	/**
	* @brief                ���˳��ĻỰ�����˳����䣬��������㲥����
	*
	* @param participant    �ѳɹ����ӵġ���Ҫ�˳����췿��ĻỰ
	*
	*/
	void leave(chat_participant_ptr participant)
	{
		participants_.erase(participant);
	}

	/**
	* @brief                �򷿼������лỰ��������
	*
	* @param msg            ��Ҫ���͵�����
	*
	*/
	void deliver(const chat_message& msg)
	{
		recent_msgs_.push_back(msg);
		while (recent_msgs_.size() > max_recent_msgs)
			recent_msgs_.pop_front();

		std::for_each(participants_.begin(), participants_.end(),
			boost::bind(&chat_participant::deliver, _1, boost::ref(msg)));
	}

private:
	std::set<chat_participant_ptr>  participants_;      ///���淿�������в���ĻỰ
	chat_message_queue              recent_msgs_;       ///���淿������Ҫ���͵���Ϣ
														//---------------------------------------------------------------------------
														// �����������������������
														//---------------------------------------------------------------------------
	enum { max_recent_msgs = 0 };
};


//==================================chat_session===============================

/**
*   @brief  �Ự��\n
*           �̳���chat_participant�࣬��Ҫ�Զ��巢����Ϣ�ķ���
*/
class chat_session
	: public chat_participant,
	public boost::enable_shared_from_this<chat_session>
{
public:

	/**
	* @brief                ��ʼ���Ự����ʱ�������뷿�䣬ֻ�гɹ�����ʱ�ż���
	*
	* @param io_service     ����ִ���첽������io_service����
	* @param room           �������
	*
	*/
	chat_session(boost::asio::io_service& io_service, chat_room& room)
		: socket_(io_service),
		room_(room)
	{
	}

	/**
	* @brief                ��ȡ�Ự���׽���
	*/
	tcp::socket& socket()
	{
		return socket_;
	}

	/**
	* @brief                ��ʼ���Ự����ʱ�������뷿�䣬ֻ�гɹ�����ʱ�ż���
	*
	* @param io_service     ����ִ���첽������io_service����
	* @param room           �������
	*
	*/
	void start()
	{
		room_.join(shared_from_this());
		boost::asio::async_read(socket_,
			boost::asio::buffer(read_msg_.data(), chat_message::header_length),
			boost::bind(
				&chat_session::handle_read_header, shared_from_this(),
				boost::asio::placeholders::error));
	}

	/**
	* @brief                ��ĳ���ͻ����յ�����Ϣ���͸��˻Ự��Ӧ�Ŀͻ���\n
	*                       �������һ����Ҫ���͵���Ϣ�󣬵���handle_write�������ʣ�����Ϣ�Ļ����������
	*
	* @param msg            �յ�����Ϣ
	*
	*/
	void deliver(const chat_message& msg)
	{
		bool write_in_progress = !write_msgs_.empty();
		write_msgs_.push_back(msg);
		if (!write_in_progress)
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(write_msgs_.front().data(),
					write_msgs_.front().length()),
				boost::bind(&chat_session::handle_write, shared_from_this(),
					boost::asio::placeholders::error));
		}
	}

	/**
	* @brief        �ڽ��ܵ���Ϣ���������ַ����������ô�������ĺ���(handle_read_body)
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_read_header(const boost::system::error_code& error)
	{
		//�յ��ͻ�����Ϣ�����1
		if (!error && read_msg_.decode_header())
		{
			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
				boost::bind(&chat_session::handle_read_body, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			room_.leave(shared_from_this());
		}
	}

	/**
	* @brief        �����������ݲ��������ַ������Ⱥ���øú���������������, �ڲ���\n
	*               �����������ȡ��ͷ, �ڶ�ȡ���������handle_read_header��������
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_read_body(const boost::system::error_code& error)
	{
		if (!error)
		{
			//�յ��ͻ�����Ϣ�����2
			room_.deliver(read_msg_);
			boost::asio::async_read(socket_,
				boost::asio::buffer(read_msg_.data(), chat_message::header_length),
				boost::bind(&chat_session::handle_read_header, shared_from_this(),
					boost::asio::placeholders::error));
		}
		else
		{
			room_.leave(shared_from_this());
		}
	}

	/**
	* @brief        ���������һ�����������ݽ�����������д�������Ϣ�����\n
	*               �ݹ���÷���handle_write����ʣ������
	*
	* @param error  ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_write(const boost::system::error_code& error)
	{
		if (!error)
		{
			write_msgs_.pop_front();
			if (!write_msgs_.empty())
			{
				boost::asio::async_write(socket_,
					boost::asio::buffer(write_msgs_.front().data(),
						write_msgs_.front().length()),
					boost::bind(&chat_session::handle_write, shared_from_this(),
						boost::asio::placeholders::error));
			}
		}
		else
		{
			room_.leave(shared_from_this());
		}
	}

private:
	tcp::socket             socket_;            ///�ûỰ���׽���              
	chat_room&              room_;              ///�ûỰ���ڵķ���
	chat_message            read_msg_;          ///��ȡ��������
	chat_message_queue      write_msgs_;        ///�ûỰ����Ҫ���͵����ݶ��� 
};

typedef boost::shared_ptr<chat_session> chat_session_ptr;


//==================================chat_server===============================

/**
*   @brief  chart_server��\n
*           ȫ��ֻ��һ������ִ���첽������io_service���󣬲���\n
*           ��ʼ����ʱ����һ�������������յ�����������
*/
class chat_server
{
public:

	/**
	* @brief                ������Ķ˵���Ϣ��������󶨣���ʼ������\n
	*                       ��client�����ӣ�������start_accept����\n
	*                       ����������������
	*
	* @param io_service     ����ִ���첽������io_service����
	* @param endpoint       Ҫ�����Ķ˿ں�
	*
	*/
	chat_server(boost::asio::io_service& io_service,
		const tcp::endpoint& endpoint)
		: io_service_(io_service),
		acceptor_(io_service, endpoint)
	{
		start_accept();
	}

	/**
	* @brief                �����첽���ղ��������յ��첽���Ӻ󣬴���һ���µĻỰ\n
	*                       ��ִ��handle_accept�����û�д���Ļ�handle_accept\n
	*                       ��������øú����Եȴ��µ��첽��������
	*
	*/
	void start_accept()
	{
		chat_session_ptr new_session(new chat_session(io_service_, room_));
		acceptor_.async_accept(new_session->socket(),
			boost::bind(&chat_server::handle_accept, this, new_session,
				boost::asio::placeholders::error));
	}

	/**
	* @brief                ������I/O����ʱû�з��ش�����ʱ�����µĻỰ����chart_room��\n
	*                       �Ự��ʽ��ʼ��ȡ�첽�ͻ��˷�������Ϣ
	*
	* @param session        ���յ��첽���������ʱ�򴴽��ĻỰ�����������I/O����ʱû��\n
	*                       ���ش�������ʼ�����첽���տͻ�����Ϣ
	* @param error          ����I/O����ʱ���صĴ�����
	*
	*/
	void handle_accept(chat_session_ptr session,
		const boost::system::error_code& error)

	{
		if (!error)
		{
			session->start();
		}

		start_accept();
	}

private:
	boost::asio::io_service&    io_service_;        ///����ִ���첽������io_service����(ȫ��ֻ�����һ��)
	tcp::acceptor               acceptor_;          ///���������յ�����������ļ�����
	chat_room                   room_;              ///���췿��
													/** ÿ�����յ��µ��������󲢵����������Ӵ���ʱ���µĻỰ���뷿�� */
};

typedef boost::shared_ptr<chat_server>  chat_server_ptr;
typedef std::list<chat_server_ptr>      chat_server_list;

//==================================LocalServer===============================

/**
*   @brief  LocalServer�࣬�̳���Node�࣬�ɱ�cocos�ڴ����ϵͳ�Զ�����\n
*/
class LocalServer
{
public:
	/**
	* @brief    ����LocalServer��������server
	*/
	static LocalServer* create(void)
	{
		LocalServer *sprite = new LocalServer();
		if (sprite)
		{
			sprite->runServer();

			return sprite;
		}
		return nullptr;
	}

	/**
	* @brief    ����һ�����̣߳�ʹserverפ����̨����
	*/
	void runServer(void)
	{
		std::thread t(&LocalServer::server, this);
		t.join();
	}

	/**
	* @brief    �ڷ����߳�������server\n
	*           ֻҪio_service�����رգ����߳̾ͻ��ں�̨�������У�ֱ�����߳̽���������
	*/
	int server(void)
	{
		try
		{
			boost::asio::io_service io_service;
			chat_server_list servers;

			using namespace std;
			tcp::endpoint endpoint(tcp::v4(), 2001);
			chat_server_ptr server(new chat_server(io_service, endpoint));
			servers.push_back(server);

			io_service.run();

		}
		catch (std::exception& e)
		{
			std::cerr << "Exception: " << e.what() << "\n";
		}

		return 0;
	}

};

int main(void)
{
	auto server = LocalServer::create();
	//auto client = Client::create(2);
	//client->_filter_mode = true;
	//client->sensitive_word = _playerName;
	//client->_with_server = true;

	return 0;
}

/*MIT License

Copyright(c)[2017][Li Kun]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
