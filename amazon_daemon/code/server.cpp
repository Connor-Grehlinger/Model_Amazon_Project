#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <errno.h>
#include <syslog.h>
#include <memory>
#include <time.h>
#include "server.h"
#include <string>
#include <google/protobuf/text_format.h>
#include <stdint.h>
#include <map>
#include <pqxx/pqxx>

using namespace pqxx;

Server::Server() {
  this->num_wh = 3;
}

void Server::init_ups(std::string ip, int port, std::string worldip, int worldport) {
  connect_ups(ip, port, worldip, worldport);
  google::protobuf::io::FileOutputStream *out_ups = new  google::protobuf::io::FileOutputStream(this->ups_socket);
  google::protobuf::io::FileInputStream *in_ups = new  google::protobuf::io::FileInputStream(this->ups_socket);
  
  //SEND WAREHOUSE INFO TO THE UPS
  AUCommands warehouse_info;
  Warehouse *wh = warehouse_info.add_whinfo();
  wh->set_whid(0);
  wh->set_whx(344);
  wh->set_why(-100);
  Warehouse *wh2 = warehouse_info.add_whinfo();
  wh2->set_whid(1);
  wh2->set_whx(844);
  wh2->set_why(-600);
  if (this->sendMesgTo(warehouse_info, out_ups) == false) {
    std::cout << "Sending warehouse info to UPS failed!" << std::endl;
  }
  std::cout << "Warehouse init Message sent to the ups! " << std::endl;
  
  //ADD PRODUCTS TO WAREHOUSE
  ACommands initial_buy;
  //initial_buy.set_simspeed(1000000);
  APurchaseMore *buy_wh1 = initial_buy.add_buy();
  buy_wh1->set_whnum(0);
  AProduct *prod1 = buy_wh1->add_things();
  prod1->set_id(1);
  prod1->set_description("Product 1 test");
  prod1->set_count(1000);
  AProduct *prod2 = buy_wh1->add_things();
  prod2->set_id(2);
  prod2->set_description("Product 2 test");
  prod2->set_count(1000);
  APurchaseMore *buy_wh2 = initial_buy.add_buy();
  buy_wh2->set_whnum(1);
  AProduct *prod3 = buy_wh2->add_things();
  prod3->set_id(1);
  prod3->set_description("Product 1 test");
  prod3->set_count(1000);
  AProduct *prod4 = buy_wh2->add_things();
  prod4->set_id(2);
  prod4->set_description("Product 2 test");
  prod4->set_count(1000);
  google::protobuf::io::FileOutputStream *out_world = new  google::protobuf::io::FileOutputStream(this->world_socket);
  google::protobuf::io::FileInputStream *in_world = new  google::protobuf::io::FileInputStream(this->world_socket);
  if (this->sendMesgTo(initial_buy, out_world) == false) {
    std::cout << "Sending Product buy message to world failed!" << std::endl;
  }
  std::cout << "Message sent to the world " << initial_buy.DebugString() << std::endl;
  AResponses buy_response;
  if (this->recvMesgFrom(buy_response, in_world) == false) {
    std::cout << "Error from the world!" << std::endl;
  }  
  std::cout << "Message received from the world " << buy_response.DebugString() << std::endl;
  AResponses buy_response2;
  if (this->recvMesgFrom(buy_response2, in_world) == false) {
    std::cout << "Error from the world!" << std::endl;
  }
  std::cout << "Message received from the world " << buy_response2.DebugString() << std::endl;  
  
  //SEND PURCHASE TO UPS
  AUCommands purchase_command;
  Purchase *p1 = purchase_command.mutable_pc();
  p1->set_whid(1);
  p1->set_x(300);
  p1->set_y(500);
  Product *product1 = p1->add_things();
  product1->set_id(1);
  product1->set_description("Product 1 test");
  product1->set_count(10);
  Product *product2 = p1->add_things();
  product2->set_id(2);
  product2->set_description("Product 2 test");
  product2->set_count(5);
  p1->set_orderid(15);
  p1->set_isprime(false);
  std::cout << "Sending purchase message to UPS " << purchase_command.DebugString() << std::endl;
  if (this->sendMesgTo(purchase_command, out_ups) == false) {
    std::cout << "Sending purchase message to UPS failed!" << std::endl;
  }
  std::cout << "Purchase command sent! " << purchase_command.DebugString() << std::endl;
  UACommands recv_truck;
  if (this->recvMesgFrom(recv_truck,in_ups) == false) {
    std::cout << "Receiving purchase confirmation from the ups failed!" << std::endl;
  }
  PackageID* pk_id = recv_truck.mutable_pid();
  order_package_map[pk_id->orderid()] = pk_id->pkgid();
  std::cout << "Ship id received: "<< recv_truck.DebugString()<<":" <<std::endl;
  
  /*
  std::string s;
  if (google::protobuf::TextFormat::PrintToString(recv_truck, &s)) {
    std::cout << "Your message: " << s << std::endl;
  } else {
    std::cout << "Something wrong with truck message!" << std::endl;
    }
  */
  
  //PACK STUFF AND SEND MESSAGE TO WORLD
  int selected_warehouse = 1;
  ACommands pack_stuff;
  APack *package_to_load = pack_stuff.add_topack();
  //pack_stuff.set_simspeed(1000000);
  package_to_load->set_whnum(selected_warehouse);
  AProduct *prod_to_pack = package_to_load->add_things();
  prod_to_pack->set_id(2);
  prod_to_pack->set_description("Product 2 test");
  prod_to_pack->set_count(5);
  AProduct *prod_to_pack2 = package_to_load->add_things();
  prod_to_pack2->set_id(1);
  prod_to_pack2->set_description("Product 1 test");
  prod_to_pack2->set_count(10);
  package_to_load->set_shipid(recv_truck.pid().pkgid());
  std::cout << "Sending stuff to pack to WORLD" << pack_stuff.DebugString();
  int64_t shipment_id = recv_truck.pid().pkgid();
  if (this->sendMesgTo(pack_stuff, out_world) == false) {
    std::cout << "Something went wrong with sending pack to world" << std::endl;
  }
  
  //GET THE RESPONSE FROM THE WORLD ABOUT TO PACK
  AResponses pack_response2;
  if (this->recvMesgFrom(pack_response2, in_world) == false) {
    std::cout << "Something went wrong with getting pack response" << std::endl;
  }
  std::cout << "Pack response2 from world received!" << pack_response2.DebugString()<<std::endl;
  AResponses pack_response;
  if (this->recvMesgFrom(pack_response, in_world) == false) {
    std::cout << "Something went wrong with getting pack response" << std::endl;
  }
  std::cout << "Pack response from world received!" << pack_response.DebugString()<<std::endl;
  for (int i = 0; i < pack_response.ready_size(); i++) {
    std::cout << "Shipment : " << i << ": " << pack_response.ready(i)<<std::endl;
    std::cout << "What I have as shipid: " << shipment_id << std::endl;
    if (shipment_id == pack_response.ready(i)) {
      std::cout << "Shipment " << i << " matched!";
    }
  }
  std::cout << "Waiting for START_LOAD from UPS!" << std::endl;
  // GET START_LOAD FROM UPS
  UACommands start_load_message;
  if (this->recvMesgFrom(start_load_message,in_ups) == false) {
    std::cout << "Receiving start load from UPS failed!" << std::endl;
  }
  std::cout << "START_LOAD Message received from UPS " << start_load_message.DebugString()<<std::endl;
  int truck_id_from_world = 0;
  for (int i = 0; i < start_load_message.sl_size(); i++) {
    StartLoad* sl_object = start_load_message.mutable_sl(i);
    //package_truck_map[sl_object->packageid()] = sl_object->truckid();
    truck_id_from_world = sl_object->truckid();
    std::cout << "SL Object "<<i<<": "<< sl_object->DebugString() << std::endl;
  }
  //START LOAD TO TRUCK
  ACommands put_to_truck;
  APutOnTruck *package_to_put = put_to_truck.add_load();
  //put_to_truck.set_simspeed(1000000);
  package_to_put->set_whnum(selected_warehouse);
  package_to_put->set_truckid(truck_id_from_world);
  package_to_put->set_shipid(shipment_id);
  std::cout << "Sending Load message to WORLD!" << put_to_truck.DebugString() << std::endl;
  if (this->sendMesgTo(put_to_truck, out_world) == false) {
    std::cout << "Something went wrong with Put truck" << std::endl;
  }

  //GET THE RESPONSE TO LOAD FROM WORLD
  AResponses load_response;
  if (this->recvMesgFrom(load_response, in_world) == false) {
    std::cout << "Something went wrong with reading put trock response" << std::endl;
  }
  std::cout << "WORLD response for LOAD is : " <<load_response.DebugString() << std::endl;
  //A little bit of checking required here when getting load response

  //SEND LOADED TO UPS
  AUCommands send_loaded_info;
  Loaded *loaded_info = send_loaded_info.mutable_ldd();
  loaded_info->set_packageid(shipment_id);
  std::cout << "Gonna send loaded message to UPS: "<< send_loaded_info.DebugString() << std::endl;
  if (this->sendMesgTo(send_loaded_info, out_ups) == false) {
    std::cout << "Sending LOADED Info to UPS failed!" << std::endl;
  }
  UACommands delivered_package;
  std::cout << "Waiting for delivered package!" << std::endl;
  if (this->recvMesgFrom(delivered_package, in_ups) == false) {
    std::cout << "Receiving delivered package from UPS failed!" << std::endl;
  }
  std::cout << "Delivered message gotten from UPS!" << delivered_package.DebugString();
  while(true);
  //GET STUFF FROM DATABASE AND PUT THEM INSIDE WAREHOUSES
}

void Server::init_world(std::string ip, int port) { // WH needs testing!
  int listenfd;
  struct sockaddr_in rmAddr; 
  struct hostent* world;
  //Create the socket for world
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Creating listen socket failed\n");
    exit(EXIT_FAILURE);
  }
  world = gethostbyname(ip.c_str());
  if (world == NULL) {
    std::cerr<< "couldn't find world!"<<std::endl;
    exit(EXIT_FAILURE);
  }
  memset(&rmAddr, '0', sizeof(rmAddr)); 
  rmAddr.sin_family = AF_INET;
  bcopy((char *)world->h_addr,
	(char *)&rmAddr.sin_addr.s_addr,
	world->h_length);
  rmAddr.sin_port = htons(port);
  //Establish connection with the ringmaster
  if (connect(listenfd, (struct sockaddr *) &rmAddr, sizeof(rmAddr)) < 0) {
    perror("Connecting to world failed!\n");
    exit(EXIT_FAILURE);
  }
  std::cout << "Succesfully connected to world!"<<std::endl;
  this->world_socket = listenfd;
  AConnect world_init;
  world_init.set_worldid(this->worldid);
  std::vector<std::unique_ptr<Whouse> >::const_iterator it;
  //NEEDS TESTING!
  for (auto& it : warehouses) {
    AInitWarehouse *wh = world_init.add_initwh();
    wh->set_x((it.get())->get_x());
    wh->set_y((it.get())->get_y());
  }
  /*
  AInitWarehouse *wh = world_init.add_initwh();
  wh->set_x(344);
  wh->set_y(-100);
  AInitWarehouse *wh2 = world_init.add_initwh();
  wh2->set_x(844);
  wh2->set_y(-600);  
  */
  google::protobuf::io::FileOutputStream *out = new  google::protobuf::io::FileOutputStream(this->world_socket);
  this->sendMesgTo(world_init, out);
  std::cout << "Message sent to world : " << world_init.DebugString() << std::endl;
  google::protobuf::io::FileInputStream *in = new  google::protobuf::io::FileInputStream(this->world_socket);
  AConnected world_response;
  this->recvMesgFrom(world_response, in);
  std::cout << "World message : " << world_response.DebugString()<< std::endl;
  delete out; //##### DELETE OUTSTREAM
  delete in; //###### DELETE INSTREAM
}

void Server::init_warehouses(void) {
  std::cout << "initializing warehouses!" << std::endl;
  for (int i = 0; i < this->num_wh; i++) {
    try {
      std::unique_ptr<Whouse> w (new Whouse(i, (i+1)*100, (i+1)*(-100)));
      warehouses.push_back(std::move(w));
    } catch (std::exception &e) {
      //log e.what
      std::cout << e.what();
    }
  }
  std::cout << "WH initialization finished!" << std::endl;
}

void Server::connect_ups(std::string ups_ip, int ups_port, std::string worldip, int worldport) {
  //TESTED, WORKS GREAT!
  int listenfd;
  struct sockaddr_in rmAddr; 
  struct hostent* ups;
  //Create the socket for UPS
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("Creating listen socket failed\n");
    exit(EXIT_FAILURE);
  }
  //Set up the server to connect UPS
  ups = gethostbyname(ups_ip.c_str());
  if (ups == NULL) {
    std::cerr<< "Server couldn't find UPS!"<<std::endl;
    exit(EXIT_FAILURE);
  }
  memset(&rmAddr, '0', sizeof(rmAddr)); 
  rmAddr.sin_family = AF_INET;
  bcopy((char *)ups->h_addr,
	(char *)&rmAddr.sin_addr.s_addr,
	ups->h_length);
  rmAddr.sin_port = htons(ups_port);
  //Establish connection with the ringmaster
  if (connect(listenfd, (struct sockaddr *) &rmAddr, sizeof(rmAddr)) < 0) {
    perror("Connecting to ups failed!\n");
    exit(EXIT_FAILURE);
  }
  this->ups_socket = listenfd;
  UACommands init_world_message;
  google::protobuf::io::FileInputStream *in_ups = new  google::protobuf::io::FileInputStream(this->ups_socket);
  this->recvMesgFrom(init_world_message, in_ups);
  std::cout << "Server received : " << init_world_message.DebugString();
  delete in_ups; // ###DELETE INPUTSTREAM
  this->worldid = init_world_message.worldid();
  std::cout << "World id: " << this->worldid << std::endl;
  this->init_world(worldip, worldport);
  std::cout << "World creation completed" << std::endl;
}

void Server::send_wh_to_ups() {
  //SEND WAREHOUSE INFO TO THE UPS  
  AUCommands warehouse_info;
  for (auto& it : warehouses) {
    Warehouse *wh = warehouse_info.add_whinfo();
    wh->set_whid((it.get())->get_id());
    wh->set_whx((it.get())->get_x());
    wh->set_why((it.get())->get_y());
  }
  std::cout << "Sending WH info to UPS!" << warehouse_info.DebugString();
  google::protobuf::io::FileOutputStream *out_ups = new  google::protobuf::io::FileOutputStream(this->ups_socket);
  if (this->sendMesgTo(warehouse_info, out_ups) == false) {
    std::cout << "Sending warehouse info to UPS failed!" << std::endl;
  }
  std::cout << "Warehouse init Message sent to the ups! " << warehouse_info.DebugString() << std::endl;
  delete out_ups;
}

void Server::initial_wh_purchase() { // TESTED!
  //ADD PRODUCTS TO WAREHOUSE
  ACommands initial_buy;
  initial_buy.set_simspeed(10000);
  pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
  nontransaction N(*amazonDB);
  std::string query = "SELECT * FROM product_product;";
  result R(N.exec(query)); 
  for (int i = 0; i < this->num_wh; i++) {
    APurchaseMore *buy_wh = initial_buy.add_buy();
    buy_wh->set_whnum(i);
    for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
      AProduct *prod = buy_wh->add_things();
      prod->set_id(c[0].as<int>());
      prod->set_description(c[2].as<std::string>());
      prod->set_count(20);
    }
  }
  N.commit();
  std::cout << "Server initial buy : "<<std::endl;
  this->world_write_queue.push(initial_buy);
  std::cout << "Server pushed initial buy to queue!" << std::endl;
}

void Server::deploy_threads() {
  this->webreader.reset(new Webreader(ups_write_queue));
  this->upswriter.reset(new Upswriter(ups_write_queue,ups_socket));
  //  this->upsreader.reset(new Upsreader(ups_read_queue,ups_socket));
}


void Server::run_server(std::string ups_ip, int ups_port, std::string worldip, int worldport) {
  this->init_warehouses();
  this->connect_ups(ups_ip, ups_port, worldip, worldport);
  this->send_wh_to_ups();
  this->webreader.reset(new Webreader(ups_write_queue));
  this->upswriter.reset(new Upswriter(ups_write_queue,ups_socket));
  this->upsreader.reset(new Upsreader(ups_read_queue,ups_socket));
  this->worldreader.reset(new Worldreader(world_read_queue,world_socket));
  this->worldwriter.reset(new Worldwriter(world_write_queue,world_socket));
  for (int i = 0; i < 8; i++) {
    try {
      std::unique_ptr<Upsworker> uw (new Upsworker(ups_read_queue,ups_write_queue,world_write_queue));
      std::unique_ptr<Worldworker> ww(new Worldworker(ups_write_queue,world_read_queue, world_write_queue));
      upsworkers.push_back(std::move(uw));
      worldworkers.push_back(std::move(ww));
    } catch (std::exception &e) {
      //log e.what
      std::cout << e.what();
    }
  }
  std::cout << "Server finished initializing and spawning threads!" << std::endl;
  this->initial_wh_purchase();
}

Server::~Server() {
  ups_write_queue.deactivate();
  ups_read_queue.deactivate();
  world_write_queue.deactivate();
  world_read_queue.deactivate();
  warehouses.clear();
  upsworkers.clear();
  worldworkers.clear();
}
