#include "webreader.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
using namespace pqxx;

Webreader::Webreader(SafeQueue<AUCommands> &q) : ups_write_queue(q),
						 web_socket(),
						 mythread(),
						 is_active(true),
						 wh_id(0)
{
  //Activate the socket
  this->web_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  struct sockaddr_in serv_addr;
  memset(&serv_addr,0,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(34567);
  //    try{
  int state = bind(this->web_socket,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
  if(state < 0){
    perror("bind error");
  }
  listen(this->web_socket,1000);
  std::cout << "Webreader constructor finishing" << std::endl;
  this->mythread = std::thread(&Webreader::listen_connections, this);
  std::cout << "Webreader thread launched!" << std::endl;
}

void Webreader::listen_connections(void) {
  try{
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    while(1){
      std::cout << "Webreader listening for connections!" << std::endl;
      int client_socket = accept(this->web_socket, (struct sockaddr *)&client_addr,&addrlen);
      if(client_socket != -1){
	try {
	  form_purchase(client_socket);
	} catch (const std::exception &e) {
	  std::cout << "WEB READER HAS RAISED AN EXCEPTION!" << e.what();
	}
      } else {
	std::cout << "Socket accept failed!"<<std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cout << "Webreader:listen_connections failed!" << e.what();
  }
}

void Webreader::form_purchase(int client_socket) {
  char buff[1024];
  memset(buff, 0, sizeof(buff));
  if (read(client_socket, buff, 1024) == -1) {
      perror("Error reading from the client!\n");
  }
  std::cout << "Form purchase received: #" << buff << "# from web app!" << std::endl;
  
  std::string orderid_string(buff);
  if (orderid_string[0] == 'C') {
    orderid_string = orderid_string.substr(1);
    int x_coordinate = std::stoi(orderid_string.substr(0,orderid_string.find(":")));
    orderid_string = orderid_string.substr(orderid_string.find(":")+1);
    int y_coordinate = std::stoi(orderid_string.substr(0,orderid_string.find(":")));
    orderid_string = orderid_string.substr(orderid_string.find(":")+1);
    int package_id = std::stoi(orderid_string);
    AUCommands changedes;
    ChangeDes *ch = changedes.mutable_chdes();
    ch->set_pkgid(package_id);
    ch->set_x(x_coordinate);
    ch->set_y(y_coordinate);
    std::cout << "Change destination object : " << changedes.DebugString() << std::endl;
    this->ups_write_queue.push(changedes);
    std::cout << "Change destination object pushed to queue!" << std::endl;
    
  } else {
    int orderid = stoi(orderid_string);
    std::cout << "Order id is: "<< orderid << std::endl;
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    nontransaction N(*amazonDB);
    std::string query = "SELECT * FROM product_order WHERE id=" +
      orderid_string + ";";
    result R(N.exec(query)); 
    result::const_iterator c = R.begin();
    int whid = wh_id; //DO BUNCH OF STUFF TO DESIGNATE A WAREHOUSE
    wh_id = (wh_id+1)%3;
    int x = c[4].as<int>();
    int y = c[5].as<int>();
    std::string upsaccount="";
    try{
      upsaccount = c[6].as<std::string>();
    } catch (const std::exception &e) {
    }
    int isprime = c[7].as<int>();
    N.commit();
  
    nontransaction N2(*amazonDB);
    query = "SELECT * FROM product_orderlist WHERE order_id_id=" +
      orderid_string + ";";
    result R2(N2.exec(query));
  
    AUCommands purchase_command;
    Purchase *p1 = purchase_command.mutable_pc();
    p1->set_whid(whid);
    p1->set_x(x);
    p1->set_y(y);
    p1->set_orderid(orderid);
    p1->set_upsuserid(upsaccount);
    if (isprime == 0) {
      p1->set_isprime(false);
    } else {
      p1->set_isprime(true);
    }
    for (result::const_iterator c2 = R2.begin(); c2 != R2.end(); ++c2) {
      Product *product1 = p1->add_things();
      int num_product = c2[1].as<int>();
      int product_id = c2[3].as<int>();
      std::string desc = c2[4].as<std::string>();
      product1->set_id(product_id);
      product1->set_description(desc);
      product1->set_count(num_product);
    }
    N2.commit();
    work orderWH(*amazonDB);
    std::string insert_orderwh = "INSERT INTO product_orderwarehouse(order_id, warehouse_id) VALUES(" + orderid_string + ","+std::to_string(whid)+");";
    orderWH.exec(insert_orderwh);
    orderWH.commit();
    std::cout << "Order warehouse object inserted into db!" << std::endl;
    std::cout << "Purchase object : " << purchase_command.DebugString() << std::endl;
    this->ups_write_queue.push(purchase_command);
    std::cout << "Purchase object pushed to queue!" << std::endl;
  }
  
}

Webreader::~Webreader() {
  this->is_active = false;
  try {
    mythread.join();
  } catch(const std::system_error &se) {
    std::cout << se.what();
    //log se.what()
  } catch (const std::exception &e) {
    //log e.what
    std::cout << e.what();
  }
}
