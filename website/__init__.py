from flask import Flask
from flask import Flask,render_template, request, flash
from flask_mysqldb import MySQL
import yaml

def create_app():
    app = Flask(__name__)
    app.config["SECRET_KEY"] = "rolothehobo"

    # mysql setup
    db = yaml.safe_load(open('db.yaml'))
    app.config['MYSQL_HOST'] = db['mysql_host']
    app.config['MYSQL_USER'] = db['mysql_user']
    app.config['MYSQL_PASSWORD'] = db['mysql_password']
    app.config['MYSQL_DB'] = db['mysql_db']

    mysql = MySQL(app)

    @app.route('/', methods=['GET', 'POST'])
    def home():
        # create connection cursor to mysql
        cursor = mysql.connection.cursor() 
        # close cursor
        cursor.close()
        return render_template("home.html")

    @app.route('/availability')
    def availability():
        cur = mysql.connection.cursor()
        resultValue = cur.execute("SELECT * FROM availability")
        if resultValue > 0:
            values = cur.fetchall() 
            availabilityDetails = []
            for idx in range(len(values)):
                status = 'Taken' if values[idx][1] == 1 else 'Available'
                availabilityDetails.append([values[idx][0], status])
            print(availabilityDetails)
            return render_template('availability.html', availabilityDetails=availabilityDetails)
        cur.close()

    @app.route('/sign-up', methods=['GET', 'POST'])
    def sign_up():
        if request.method == "POST":
            # Get email and rack subscriber info from user
            email = str(request.form.get('email'))
            rack_1 = 0 if not request.form.get('rack1') else 1
            rack_2 = 0 if not request.form.get('rack2') else 1
            rack_3 = 0 if not request.form.get('rack3') else 1
            rack_4 = 0 if not request.form.get('rack4') else 1

            print(email, rack_1, rack_2, rack_3, rack_4)

            # Send subscriber data to mysql db
            # create connection cursor to mysql
            con = mysql.connection
            cur = con.cursor()
            
            if email:
                # If email not in db, insert row into db with email and user's notification preferences
                email_exist_db = cur.execute("SELECT 1 FROM rack_subscriptions WHERE email_address= '{}'".format(email))
                if not email_exist_db:
                    sql_insert = "INSERT INTO rack_subscriptions (email_address, rack_1, rack_2, rack_3, rack_4) VALUES ('{}', {}, {}, {}, {})".format(email, rack_1, rack_2, rack_3, rack_4)
                    cur.execute(sql_insert)
                    con.commit()
                    print("data received and SET user's notification preferences in MySQL") 
                    flash("Email notification preferences have been set!", category="success")
                # If email exists in db, then update user's notification preferences
                else:
                    sql_insert = "UPDATE rack_subscriptions SET rack_1={}, rack_2={}, rack_3={}, rack_4={} WHERE email_address='{}'".format(rack_1, rack_2, rack_3, rack_4, email)
                    cur.execute(sql_insert)
                    con.commit()
                    print("data received and UPDATED user's notification preferences in MySQL") 
                    flash("Email notification preferences have been updated!", category="success")
            else:
                flash("NO EMAIL PROVIDED", category="error")

            cur.close()

        return render_template("sign_up.html")
    # from .views import views
    # app.register_blueprint(views, url_prefix='/')
    
    return app
