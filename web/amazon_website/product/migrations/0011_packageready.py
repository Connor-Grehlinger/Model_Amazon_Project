# Generated by Django 2.0.1 on 2018-04-26 15:03

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('product', '0010_ordershipment_ordershipmenttruck'),
    ]

    operations = [
        migrations.CreateModel(
            name='PackageReady',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('package_id', models.IntegerField()),
                ('ready', models.IntegerField()),
            ],
        ),
    ]
