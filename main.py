from venv import create
from website import create_app

# website is a python package
# this is because __init__.py is within website, making it a package

app = create_app()

# only run webserver when main.py is run directly
if __name__ == '__main__':
    # debug=True -> any time change to python code made, rerun app
    app.run(debug=True)