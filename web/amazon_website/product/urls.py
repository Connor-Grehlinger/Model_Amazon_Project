from django.urls import path
from . import views
urlpatterns = [
    path('product_page/<int:product_id>', views.product_page, name='product_page'),
    path('cart_page', views.cart_page, name='cart_page'),
    path('delete_cart/<int:product_id>', views.delete_cart, name='delete_cart'),
    path('change_destination/<int:orderid>', views.change_destination, name='change_destination'),
    path('list_orders', views.list_orders, name='list_orders'),

]
