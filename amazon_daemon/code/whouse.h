class Whouse {
 private :
  int id;
  int x;
  int y;
 public:
 Whouse(int id, int x, int y): id(id), x(x), y(y){}
  ~Whouse(){}
  const int get_x() const {
    return x;
  }
  const int get_y() const {
    return y;
  }
  const int get_id() const {
    return id;
  }
};
