#include "upsworker.h"
#include <chrono>

using namespace pqxx;

Upsworker::Upsworker(SafeQueue<UACommands> &q1, SafeQueue<AUCommands> &q2, SafeQueue<ACommands> &q3) : ups_read_queue(q1),
											     ups_write_queue(q2),
											     world_write_queue(q3),
											     mythread(),
											     is_active(true)
{
  std::cout << "Upsworker constructor finishing" << std::endl;
  this->mythread = std::thread(&Upsworker::parse_message, this);
  std::cout << "Upsworker thread launched!" << std::endl;
}

void Upsworker::parse_message(void) {
  while(this->is_active) {
    std::cout << "Ups worker trying to pop something out of queue!" << std::endl;
    UACommands ups_in_message;
    if (!ups_read_queue.try_pop(ups_in_message)) {
      this->is_active = false;
      continue;
    }
    try {
      std::cout << "Ups worker popped this!" << ups_in_message.DebugString();
      if (ups_in_message.pks_size() != 0) { // Package status
	for (int i = 0; i < ups_in_message.pks_size(); i++) {
	  PackageStatus* package = ups_in_message.mutable_pks(i);
	  update_package_status(package->pkgid(),package->status());
	}
      } else if (ups_in_message.sl_size() != 0) { // STARTLOAD
	ACommands put_to_truck;
	put_to_truck.set_simspeed(10000);
	bool is_broken = false;
	for (int i = 0; i < ups_in_message.sl_size(); i++) {
	  StartLoad* sl_object = ups_in_message.mutable_sl(i);
	  std::cout << "SL Object in startload "<<i<<": "<< sl_object->DebugString() << std::endl;
	  for (int i = 0; i < sl_object->packageid_size(); i++) {
	    //Check if product ready, if not put it pack to UPS Read queue
	    long int packageid = sl_object->packageid(i);
	    if (this->check_package_ready(std::to_string(packageid), ups_in_message) == false) {
	      is_broken = true;
	      std::cout << "Package is not ready!" << std::endl;
	      break;
	    }
	    std::cout << "Loading package message here!" << std::endl;
	    APutOnTruck *package_to_put = put_to_truck.add_load();
	    package_to_put->set_whnum(sl_object->whid());
	    package_to_put->set_truckid(sl_object->truckid());
	    package_to_put->set_shipid(packageid);
	  }
	  if (is_broken) { //Think exceptions
	    std::cout << "Package was not ready!" <<std::endl;
	    break;
	  }
	}
	if (is_broken) {
	  std::cout << "Package was not ready, not putting to queue"<<std::endl;
	  continue;
	}
	std::cout << "Putting load message to worldqueue!" << put_to_truck.DebugString();
	this->world_write_queue.push(put_to_truck);
	std::cout << "Upsworker pushed LOAD to world write queue!" << put_to_truck.DebugString();

      } else if(ups_in_message.has_dcr()) {
	//ChangeDes message!
	std::cout << "Change destination confirmation received!" << ups_in_message.DebugString();

      } else if (ups_in_message.has_pid()) { //Buy message response!
	PackageID* pk_id = ups_in_message.mutable_pid();
	this->insert_ordershipment(std::to_string(pk_id->orderid()), std::to_string(pk_id->pkgid()));
	std::cout << "Ups worker preparing pack message!" << std::endl;
	ACommands pack_stuff;
	APack *package_to_load = pack_stuff.add_topack();
	pack_stuff.set_simspeed(10000); //WILL BE CHANGED!
	int whnum = this->get_warehouse(pk_id->orderid());
	package_to_load->set_whnum(whnum);
	package_to_load->set_shipid(pk_id->pkgid());
	ACommands buy_stock;
	APurchaseMore *stock = buy_stock.add_buy();
	stock->set_whnum(whnum);
	pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
	nontransaction N2(*amazonDB);
	std::string query = "SELECT * FROM product_orderlist WHERE order_id_id=" + std::to_string(pk_id->orderid()) + ";";
	result R2(N2.exec(query));
	for (result::const_iterator c2 = R2.begin(); c2 != R2.end(); ++c2) {
	  int num_product = c2[1].as<int>();
	  int product_id = c2[3].as<int>();
	  std::string desc = c2[4].as<std::string>();
	  AProduct *prod_to_pack = package_to_load->add_things();
	  prod_to_pack->set_id(product_id);
	  prod_to_pack->set_description(desc);
	  prod_to_pack->set_count(num_product);
	  AProduct *prod_to_buy = stock->add_things();
	  prod_to_buy->set_id(product_id);
	  prod_to_buy->set_description(desc);
	  prod_to_buy->set_count(num_product);
	}
	N2.commit();
	delete amazonDB;
	this->insert_packageready(std::to_string(pk_id->pkgid()));
	this->update_trackingno(std::to_string(pk_id->orderid()), std::to_string(pk_id->pkgid()));
	std::cout << "World restock message prepared!" << buy_stock.DebugString();
	this->world_write_queue.push(buy_stock);
	std::cout << "World buy message put into world write queue!" << std::endl;
	this->check_stock_buy(pack_stuff);
	std::cout << "World pack message prepared!" << pack_stuff.DebugString();
	this->world_write_queue.push(pack_stuff);
	std::cout << "World pack message put into world write queue!" << std::endl;
	this->reduce_from_warehouse(pack_stuff);
      } else if (ups_in_message.has_disconnect()) {
	std::cout << "GOT DISCONNECT FROM UPS!" << ups_in_message.DebugString();
      } else {
	std::cout << "Could not parse message from UPS!" << ups_in_message.DebugString();
      }
    } catch (const std::exception &e) {
      std::cout<<"general therad exception!" << e.what();
    }
  }
}

bool Upsworker::check_package_ready(std::string package_id, UACommands ups_in_message) {
  try{
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    nontransaction N2(*amazonDB);
    std::string query = "SELECT * FROM product_packageready WHERE package_id=" +
      package_id + ";";
    result R2(N2.exec(query));
    result::const_iterator c = R2.begin();
    std::cout << "Checking if package is ready! " << c[2].as<int>() << "is the value!" << std::endl;
    if (c[2].as<int>() == 0) {
      std::cout << "Package was not ready! Will sleep 5 seconds"<< std::endl;
      std::chrono::seconds duration(5);
      std::this_thread::sleep_for(duration);
      std::cout << "Woke up from 5 seconds of sleep!" << std::endl;
      this->ups_read_queue.push(ups_in_message);
      std::cout << "Package was not ready! Pushed it back to queue"<< std::endl;
      return false;
    }
    N2.commit();
    return true;
  } catch (const std::exception &e) {
    std::cout << "check package ready EXCEPTION" << e.what();
    return false;
  }
}

void Upsworker::update_package_status(int packageid, std::string status) {
  try {
  pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
  nontransaction N(*amazonDB);
  std::string query = "SELECT * FROM product_ordershipment where package_id=" + std::to_string(packageid)+";";
  result R2(N.exec(query));
  result::const_iterator c = R2.begin();
  int order_id = c[1].as<int>();
  N.commit();
  work W(*amazonDB);
  std::string query2 ="UPDATE product_order SET status=\'" + status + "\' WHERE id="+std::to_string(order_id)+";";
  W.exec(query2);
  W.commit();
  std::cout << "Status updated for order: " << order_id <<" as : "<< status << std::endl; 
  } catch(const std::exception &e) {
    std::cout << "UPDATE PACKAGE STATUS EXCEPTION" <<e.what();
  }
}

int Upsworker::get_warehouse(int orderid) {
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    nontransaction getWH(*amazonDB);
    std::string wh_query = "SELECT * FROM product_orderwarehouse WHERE order_id=" +std::to_string(orderid) +";";
    result R2(getWH.exec(wh_query));
    result::const_iterator c = R2.begin();
    int order_id = c[2].as<int>();
    getWH.commit();
    return order_id;
  } catch(const std::exception &e) {
    std::cout<<"GET WAREHOUSE EXCEPTION" << e.what();
    return -1;
  }
}

void Upsworker::insert_ordershipment(std::string orderid, std::string pkid) {
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    work W(*amazonDB);
    std::string insert_query = "INSERT INTO product_ordershipment(order_id, package_id) VALUES(" + orderid + "," + pkid + ");";
    W.exec(insert_query);
    W.commit();
    std::cout << "Ups worker put ship id to DB!" <<std::endl;
  } catch (const std::exception &e) {
    std::cout <<"INSERT ORDERSHIPMENT EXCEPTION"<< e.what();
  }
}

void Upsworker::insert_packageready(std::string packageid) {
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    work W2(*amazonDB);
    std::string insert_query2 = "INSERT INTO product_packageready(package_id, ready) VALUES(" + packageid+ ",0);";
    W2.exec(insert_query2);
    W2.commit();
    std::cout << "Package not ready put into DB!" << std::endl;
  } catch (const std::exception &e) {
    std::cout <<"INSERT PACKAGEREADY EXCEPTION" << e.what();
  }
}

void Upsworker::update_trackingno(std::string orderid, std::string pkid) {
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    work W3(*amazonDB);
    std::string query3 ="UPDATE product_order SET tracking_no=" +pkid+ " WHERE id=" +orderid+";";
    W3.exec(query3);
    W3.commit();
    std::cout << "Tracking number updated in DB!" << std::endl;
  } catch (const std::exception &e) {
    std::cout << "UPDATE TRACKINGNO EXCEPTION" <<e.what();
  }
}

void Upsworker::check_stock_buy(ACommands &pack_message) {
  try {
    bool stock_unready = true;
    while (stock_unready) {
      
      bool is_broken = false;
      for (int i = 0; i < pack_message.topack_size(); i++) {
	APack package = pack_message.topack(i);
	int whnum = package.whnum();
	for (int j = 0; j < package.things_size(); j++) {
	  AProduct product = package.things(j);
	  pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
	  nontransaction N2(*amazonDB);
	  std::string query = "SELECT * FROM product_warehousestock WHERE whid=" + std::to_string(whnum) + " AND product_id="+std::to_string(product.id())+";";
	  result R2(N2.exec(query));
	  result::const_iterator c = R2.begin();
	  if (c == R2.end()) {
	    is_broken = true;
	    N2.commit();
	    delete amazonDB;
	    std::cout << "not ready, thread wills sleep for 5" << std::endl;
	    std::this_thread::sleep_for(std::chrono::seconds(5));
	    break;
	  }
	  if (c[1].as<int>() < product.count()) {
	    is_broken = true;
	    N2.commit();
	    delete amazonDB;
	    std::cout << "needs restocking, thread wills sleep for 5" << std::endl;
	    std::this_thread::sleep_for(std::chrono::seconds(5));
	    break;
	  }
	  N2.commit();
	  delete amazonDB;
	}
	if (is_broken) {
	  break;
	}
      }
      if (is_broken) {
	continue;
      }
      stock_unready = false;
    }
  } catch (const std::exception &e) {
    std::cout<<"RESTOCKING EXCEPTION" << e.what();
  }
}

void Upsworker::reduce_from_warehouse(ACommands &pack_message) {
  try {
    pqxx::connection * amazonDB = new pqxx::connection("host=amazon_db dbname=postgres user=postgres password=youshallnotpass port=5432");
    for (int i = 0; i < pack_message.topack_size(); i++) {
      APack package = pack_message.topack(i);
      int whnum = package.whnum();
      for (int j = 0; j < package.things_size(); j++) {
	AProduct product = package.things(j);
	work W(*amazonDB);
	std::string query ="UPDATE product_warehousestock SET in_stock= product_warehousestock.in_stock +" +std::to_string(product.count())+ " WHERE whid=" +std::to_string(whnum)+" AND product_id="+std::to_string(product.id())+";";
	W.exec(query);
	W.commit();
      }
    }
  } catch (const std::exception &e) {
    std::cout << "REDUCE FROM WAREHOUSE EXCEPTION" << e.what();
  }
}

Upsworker::~Upsworker() {
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
