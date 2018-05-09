from django.shortcuts import render, redirect, get_object_or_404, get_list_or_404
from product.models import *
from .forms import *
from django.contrib import messages
from django.http import HttpResponseRedirect
from django.template.defaulttags import register
import operator
from functools import reduce
from django.db.models import Q
import json
import socket
import sys
from django.core.mail import EmailMessage

# Create your views here.

def list_products(request):
    cart_num = 0
    current_user = request.user
    current_profile = None
    if current_user.is_authenticated:
        current_profile = Profile.objects.get(user=current_user)
        cart_num = Cart.objects.filter(user_id=current_profile).count()
    if request.method == 'POST':
        if 'search' in request.POST:
            form = SearchForm(request.POST)
            if form.is_valid():
                search_form = form.cleaned_data
                query = search_form['query'].split()
                if not query:
                    return redirect('/')
                product_list = Product.objects.filter(reduce(operator.or_,(Q(name__contains=x) for x in query)))
                if product_list.count() == 0:
                    messages.warning(request, "No results found!")
                    product_list = Product.objects.all()[:10]
        else:
            messages.warning(request, "Something went wrong!")
            return redirect('/')
    else:
        form = SearchForm()
        product_list = Product.objects.all()[:10]
        
    return render(request, 'home/products.html', {'plist' : product_list,
                                                  'form' : form,
                                                  'cart_num' : cart_num})

def product_page(request, product_id):
    current_product = get_object_or_404(Product, pk=product_id)
    current_user = request.user
    current_profile = None
    cart_num = 0
    if current_user.is_authenticated:
        current_profile = Profile.objects.get(user=current_user)
        cart_num = Cart.objects.filter(user_id=current_profile).count()
    else:
        pass
    if request.method == 'POST':
        if 'back' in request.POST:
            return redirect('/')
        form = ProductForm(request.POST, num_choices=current_product.max_amount)
        if form.is_valid():
            product_form = form.cleaned_data
            num_wanted = int('0' + product_form['quantity'])
            if (Cart.objects.filter(user_id=current_profile,product_id=current_product).count() == 0):
                c_instance = Cart.objects.create(user_id=current_profile,product_id=current_product,num_product=num_wanted)
                c_instance.save()
                cart_num += 1
                messages.success(request, 'Item added to cart!')
            else:
                prev_cartobj = Cart.objects.filter(user_id=current_profile,product_id=product_id)[0]
                total_num = prev_cartobj.num_product + num_wanted
                Cart.objects.filter(user_id=current_profile,product_id=product_id).update(num_product=total_num)
                messages.success(request, 'Item in the cart updated!')
        else:
            messages.warning(request,'Something went wrong with product page')
    else :
        form = ProductForm(num_choices=current_product.max_amount)    
    return render(request, 'transaction/buypage.html', {'product' : current_product,
                                                        'form' : form,
                                                        'cart_num' : cart_num})

def cart_page(request) :
    current_user = request.user
    current_profile = None
    discount = 100.0
    if 'discount' in request.session:
        discount = request.session['discount']
    if current_user.is_authenticated:
        current_profile = Profile.objects.get(user=current_user)
        list_products = Cart.objects.filter(user_id=current_profile)
        cart_num = list_products.count()
        price_beforebuy = 0
        for cart_item in list_products:
            price_beforebuy += cart_item.product_id.price * cart_item.num_product
        price_beforebuy = price_beforebuy * (discount/100.0)
    else:
        pass
    if (cart_num == 0):
        messages.warning(request, 'There are no items in the cart!')
        return redirect('/')
    if request.method == 'POST':
        if 'back' in request.POST:
            return redirect('/')
        if 'coupon' in request.POST:
            couponform = CouponForm(request.POST)
            if couponform.is_valid():
                couponcode = couponform.cleaned_data['coupons']
                percentage_obj = Coupon.objects.filter(coupon=couponcode).first()
                if percentage_obj:
                    request.session['discount'] = 100.0-percentage_obj.discount
                    messages.success(request, "Discount code succesfully applied for :" + str(percentage_obj.discount) + "% !")
                else:
                    messages.warning(request, "Discount code not found!")
                    if 'discount' in request.session:
                        del request.session['discount']
            return redirect('/cart_page')
        if 'Checkout' in request.POST:
            form = AddressForm(request.POST)
            if form.is_valid():
                address_form = form.cleaned_data
                x_coordinate = int(address_form['x'])
                y_coordinate = int(address_form['y'])
                ups_account = address_form['ups_account']
                is_prime = address_form['is_prime']
                if not is_prime:
                    is_prime = 0
                if ups_account:
                    order_instance = Order.objects.create(user_id=current_profile, status="In warehouse", x_coordinate=x_coordinate,y_coordinate=y_coordinate, ups_account=ups_account, is_prime=is_prime)
                else:
                    order_instance = Order.objects.create(user_id=current_profile, status="In warehouse", x_coordinate=x_coordinate,y_coordinate=y_coordinate, is_prime=is_prime)
                order_instance.save()
                total = 0
                for product in list_products:
                    order_item = OrderList.objects.create(order_id=order_instance,product_id=product.product_id,num_product=product.num_product, description=product.product_id.description)
                    order_item.save()
                    total += order_item.product_id.price * order_item.num_product
                Order.objects.filter(pk=order_instance.pk).update(price=(total*discount/100.0))
                Cart.objects.filter(user_id=current_profile).delete()
                messages.success(request,'Successfully ordered items in the cart!')
                alert_daemon(order_instance.pk)
                send_email(current_user.email, current_user.username)
                if 'discount' in request.session:
                        del request.session['discount']
                return redirect('/')
            else :
                messages.warning(request,'Address provided is not valid!')
                return render(request, 'transaction/cartpage.html', {'products' : list_products,
                                                                     'form' : form,
                                                                     'cart_num' : cart_num,
                                                                     'price' : price_beforebuy})
    else:
        form = AddressForm()
        couponform = CouponForm()
    return render(request, 'transaction/cartpage.html', {'products' : list_products,
                                                         'form' : form,
                                                         'cart_num' : cart_num,
                                                         'price' : price_beforebuy,
                                                         'couponform' : couponform})

def delete_cart(request, product_id):
    current_product = get_object_or_404(Product, pk=product_id)
    current_user = request.user
    if current_user.is_authenticated:
        current_profile = Profile.objects.get(user=current_user)
        cart_object = get_object_or_404(Cart, user_id=current_profile, product_id=current_product)
        cart_object.delete()
        messages.success(request, 'Item succesfully deleted from the cart!')
    else:
        pass
    return redirect('/cart_page')
"""    next = request.POST.get('next', '/')
    return HttpResponseRedirect(next)
"""

def list_orders(request):
    current_user = request.user
    order_contents = []
    if current_user.is_authenticated:
        current_profile = Profile.objects.get(user=current_user)
        list_products = Cart.objects.filter(user_id=current_profile)
        cart_num = list_products.count()
        order_list = Order.objects.filter(user_id=current_profile)
        for current_order in order_list:
            order_contents.append((current_order,OrderList.objects.filter(order_id=current_order.pk)))
    else:
        pass
    if request.method == 'POST':
        if 'back' in request.POST:
            pass
        else:
            messages.warning(request, "An error has happened")
        return redirect('/')
    return render(request, 'transaction/listorders.html', {'orders' : order_list,
                                                           'order_contents' : order_contents,
                                                           'cart_num' : cart_num})


def order_page(request, order_id):
    current_user = request.user
    if current_user.is_authenticated:
        current_profile = Profile.objects.get(user=current_user)
        list_products = Cart.objects.filter(user_id=current_profile)
        cart_num = list_products.count()
        current_order = Order.objects.get(pk=order_id)
    else:
        pass
    return render(request, 'transaction/order_page.html', {'order' : current_order,
                                                        'cart_num' : cart_num})

def change_destination(request, orderid):
    current_user = request.user
    current_order = Order.objects.get(pk=orderid)
    if current_user.is_authenticated:
        if request.method == 'POST':
            if 'back' in request.POST:
                return redirect('/list_orders')
            if 'change' in request.POST:
                address_form = ChangeAddrForm(request.POST)
                if address_form.is_valid():
                    new_x = address_form.cleaned_data['x']
                    new_y = address_form.cleaned_data['y']
                    if new_x and new_y:
                        try:
                            pckgid = OrderShipment.objects.get(order_id=orderid)
                            packageid = pckgid.package_id
                            alert_daemon("C"+str(new_x)+":"+str(new_y)+":"+str(packageid))
                            messages.success(request, "Address succesfully changed!")
                            Order.objects.filter(pk=orderid).update(x_coordinate=new_x, y_coordinate=new_y)
                        except:
                            messages.warning(request, "That order was not assigned a package id yet!")
                            return redirect('/list_orders')
                    else:
                        messages.warning(request, "Bad addres given!")
                    return redirect('/list_orders')
        else:
            address_form = ChangeAddrForm()
        return render(request, 'transaction/change_dest.html', {'address_form' : address_form, 'order':current_order})
    messages.warning(request, "Need to log in to see this page!")
    return redirect('/')
        
@register.filter
def get_item(dictionary, key):
        return dictionary.get(key)

def alert_daemon(order_id):
    """data = {'order_id' : order_id} 
    json_data = json.dumps(data)
    print(json_data)
    """
    data = str(order_id)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        print ("Sending :" + data)
        sock.connect(("amazon_daemon",34567))
        print("connected to socket!")
        sock.send(data.encode())
        print("Sent data to server!")
        sock.close()
    except:
        print("Socket comm failed!")

def send_email(email, uname):
    #send email
    try:
        body = "Hello, " + uname + ",your order has been confirmed!"
        emails = EmailMessage("Ardazon order confirmation", body, to=[email])
        emails.send()
    except:
        print("Sending order confirmation email failed!")
