from django.db import models
from account.models import Profile
# Create your models here.

class Product(models.Model):
    name = models.CharField(max_length=500, default="Product name")
    description = models.CharField(max_length=2000, default ="Product description")
    seller = models.CharField(max_length=2000, default ="Product seller")
    max_amount = models.IntegerField(default=50)
    price = models.IntegerField(default=0)
    def __str__(self):
        return self.name

class Cart(models.Model):
    user_id = models.ForeignKey(Profile, on_delete=models.CASCADE)
    product_id = models.ForeignKey(Product, on_delete=models.CASCADE)
    num_product = models.IntegerField(default=1)
    def __str__(self):
        return self.user_id.user.username + ' ' + self.product_id.name + ' ' + str(self.num_product) 

class Order(models.Model):
    user_id = models.ForeignKey(Profile, on_delete=models.CASCADE, blank=True, null=True)
    status  = models.CharField(max_length=500, default ="Product status: In warehouse - Out for delivery - Delivered - Cancelled")
    tracking_no = models.IntegerField(blank=True, null=True)
    ups_account = models.CharField(max_length=100,blank=True, null=True)
    x_coordinate = models.IntegerField(default=1)
    y_coordinate = models.IntegerField(default=1)
    is_prime = models.IntegerField(default=0)
    price = models.IntegerField(default=0)
    time = models.DateTimeField('time of the event', blank=True, null=True)

class OrderList(models.Model):
    order_id = models.ForeignKey(Order, on_delete=models.CASCADE)
    product_id = models.ForeignKey(Product, on_delete=models.CASCADE)
    num_product = models.IntegerField(default=1)
    description = models.CharField(max_length=2000, default ="Product description")

class OrderShipment(models.Model):
    order_id = models.IntegerField()
    package_id = models.IntegerField()

class PackageReady(models.Model):
    package_id = models.IntegerField()
    ready = models.IntegerField()

class OrderWarehouse(models.Model):
    order_id = models.IntegerField()
    warehouse_id = models.IntegerField()

class Coupon(models.Model):
    coupon = models.CharField(max_length=100, blank=True, null=True)
    discount = models.IntegerField()

class WarehouseStock(models.Model):
    whid = models.IntegerField()
    product = models.ForeignKey(Product, on_delete=models.CASCADE)
    in_stock = models.IntegerField()
    class Meta:
        unique_together = ('whid','product',)
