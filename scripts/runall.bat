rem Make sure you have created a virtual environment with all the requirements

call .venv\Scripts\activate.bat
pip install -r requirements.txt

python convertmap.py
python convertimages.py
python convertfx.py

echo "ALL COMPLETE!"
