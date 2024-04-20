![2D-Tile](https://github.com/Bubbq/2D-World-Builder/assets/134325235/d725781a-7e17-4198-90af-a0b0dfa05cb4)

What is It?
- A 2D editor that allows you to create a basic world through basic tiles drawn from any png you upload that any game has, like walls, floors, doors, interactable items, etc 

How Does It Work?

- I made a layer system that identifies each tile you draw with some world property to make world building compatible with any image (Note: works best on 16px asset packs). An example of this engine being used is here:
![engine](https://github.com/Bubbq/2D-World-Builder/assets/134325235/b3657213-f643-4962-a3ef-6c49c608ce34)
every wall tile during game loop is checking if the players position collides with it, if so, collision functionality occurs

How Can I Run It?
- Clone the repo (“git clone LINK” in terminal)
Compile the code and run "make && run" in terminal to move player around and "make editor && run" to edit a world

What Are The Controls? <br>
- E key toggles from free view to edit mode <br>
- Right click and drag to move world around (in free view mode) <br>
- Right Click for tile deletion (in edit mode) <br>
- Left Click to add tile to world <br>
- A key to erase all tiles in the world <br>