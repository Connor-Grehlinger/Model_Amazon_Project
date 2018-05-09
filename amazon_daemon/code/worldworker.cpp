#include "worldworker.h"
using namespace pqxx;
Worldworker::Worldworker(SafeQueue<AUCommands> &q1, SafeQueue<AResponses> &q2, SafeQueue<ACommands> &q3) : ups_write_queue(q1),
													   world_read_queue(q2),
													   world_write_queue(q3),
													   mythread(),
													   is_active(true)
{
  std::cout << "Worldworker constructor finishing" << std::endl;
  this->mythread = std::thread(&Worldworker::parse_message, this);
  std::cout << "Worldworker thread launched!" << std::endl;
}

void Worldworker::parse_message(void) {
  while(this->is_active) {
    AResponses world_in_message;
    std::cout << "World worker trying to pop something out of queue!" << std::endl;
    if (!world_read_queue.try_pop(world_in_message)) {
      this->is_active = false;
      continue;
    }
    try {
      std::cout << "World worker popped this!" << world_in_message.DebugString();
      if (world_in_message.loaded_size() != 0) {
	//LOADED TO TRUCK MESSAGE!
	std::cout << "Loaded message arrived!" << std::endl; 
	for (int i = 0; i < world_in_message.loaded_size(); i++) {
	  long int packageid = world_in_message.loaded(i);
	  AUCommands send_loaded_info;
	  Loaded *loaded_info = send_loaded_info.mutable_ldd();
	  loaded_info->set_packageid(packageid);
	  std::cout << "World worker putting Loaded to UPS write queue! "<< send_loaded_info.DebugString() << std::endl;
	  this->ups_write_queue.push(send_loaded_info);
	  std::cout << "World worker put to write queue loaded!"<< send_loaded_info.DebugString() << std::endl;
	}
      } else if (world_in_message.ready_size() != 0) {
	//PACKING IS READY MESSAGE!
	std::cout << "PACKING IS READY FROM WORLD!" << std::endl;
	for (int i = 0; i < world_in_message.ready_size(); i++) {
	  long int packageid = world_in_message.ready(i);
	  this->update_ready(packageid);
	}
      } else if (world_in_message.has_error()) {
	//MESSAGE HAS ERROR!
	std::cout << "World responded with an error!" << world_in_message.DebugString();
      } else if (world_in_message.has_finished()) {
	//FINISHED MSG, DON'T KNOW WHAT THOUGH
	std::cout << "Received finished, don't know what it means!" << world_in_message.DebugString();
      } else if (world_in_message.arrived_size() != 0) { //Arrived message!
	std::cout << "Buy received!" << std::endl;
	this->update_stock_db(world_in_message);
      }
      else {
	std::cout << "Could not parse message from UPS!" << world_in_message.DebugString();
      }
    } catch (const std::exception &e) {
      std::cout <<"Worldworker GENERAL EXCEPTION" <<e.what();
    }
  }
}

void Worldworker::update_ready(long int packageid) {
  std::string packageid_str = std::to_string(packageid);
  std::cout <<"READY RECEIVED, GONNA UPDATE PACKAGE INFO IN DB!" << std::endl;
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    work W(*amazonDB);
    std::string query2 ="UPDATE product_packageready SET ready=1 WHERE package_id="+packageid_str+";";
    W.exec(query2);
    W.commit();
    std::cout << "Ready updated for package: " << packageid << std::endl;
  } catch(const std::exception &e) {
    std::cout<<"WORLDWORKER UPDATE READY EXCEPTION" << e.what();
  }
}

void Worldworker::update_stock_db(AResponses &world_in_message) {
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    work W(*amazonDB);
    for (int i = 0; i < world_in_message.arrived_size(); i++) {
      APurchaseMore arrived = world_in_message.arrived(i);
      int whnum = arrived.whnum();
      for (int j = 0; j < arrived.things_size(); j++) {
	AProduct product = arrived.things(j);
	std::string query = "INSERT INTO product_warehousestock (whid,in_stock,product_id) VALUES ("+std::to_string(whnum)+","+std::to_string(product.count())+","+std::to_string(product.id())+") ON CONFLICT (whid,product_id) DO UPDATE SET in_stock = product_warehousestock.in_stock + excluded.in_stock ;";
	W.exec(query);
      }
    }
    W.commit();
  } catch (const std::exception &e) {
    std::cout<<"UPDATE STOCK DB EXCEPTION!" << e.what();
  }
}

Worldworker::~Worldworker() {
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
