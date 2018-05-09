from django.contrib import admin
from product.models import *
# Register your models here.

admin.site.register(Product)
admin.site.register(Cart)
admin.site.register(Order)
admin.site.register(OrderList)
admin.site.register(OrderShipment)
admin.site.register(PackageReady)
admin.site.register(OrderWarehouse)
admin.site.register(WarehouseStock)
admin.site.register(Coupon)
