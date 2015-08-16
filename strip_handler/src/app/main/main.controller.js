class MainController {
  constructor () {
    'ngInject';
    this.pixels = [];
    this.stripLength = 60;
    this.selectedColor = "#FFFFFF";
    for (var i = 0; i < this.stripLength; i++) {
      this.pixels.push("#000000");
    }
  }

  updatePixel(index) {
    this.pixels[index] = this.selectedColor;
  }

  clearAll() {
    for (var i = 0; i < this.stripLength; i++) {
      this.pixels[i] = "#000000";
    }
  }
}

export default MainController;
