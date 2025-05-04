#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Constants
const int Screen_width = 128;
const int Screen_ht = 64;
const int paddle_width = 20;
const int paddle_height = 4;
const int ball_radius = 2;

#define UP_BUTTON 4
#define DOWN_BUTTON 5
#define LEFT_BUTTON 16
#define RIGHT_BUTTON 18
#define RESET_BUTTON 25

// Ball position and speed
int ball_x = 64, ball_y = 32;
int ball_x_speed = 1, ball_y_speed = 1;

// Paddle positions
int paddle1_x = 55, paddle1_y = 60; // Player 1 (bottom)
int paddle2_x = 55, paddle2_y = 0;  // Player 2 (top)

// Scores
int score1 = 0, score2 = 0;

// Button states
int player1_left, player1_right, player2_left, player2_right;

// Game modes
enum GameMode { SINGLE_PLAYER, DUAL_PLAYER, PLAYER_VS_AI };
GameMode game_mode = SINGLE_PLAYER;

enum GameOption { PING_PONG, SNAKE_VENTURE};
GameOption game_option = PING_PONG;

// Restart button pin
const int restart_button_pin = 25;

// Ball collision flags
bool ball_hit_paddle1 = false;
bool ball_hit_paddle2 = false;

// Game variables
int snakeX[100] = {64}; // X coordinates of the snake body
int snakeY[100] = {32}; // Y coordinates of the snake body
int snakeLength = 5;    // Initial length of the snake
int foodX, foodY;       // Food position
int dirX = 1, dirY = 0; // Snake direction (initially moving right)
bool gameOver = false;

// Function prototypes
void generateFood();
void updateSnake();
void checkCollision();
void drawGame();
void readButtons();
void resetGame();
void displayGameOver();
bool isButtonPressed(int pin);
void selectModes();
void selectMode();


// Initialize display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup() {
  pinMode(16, INPUT_PULLDOWN); // Player 1 left
  pinMode(18, INPUT_PULLDOWN); // Player 1 right
  pinMode(4, INPUT_PULLDOWN);  // Player 2 left
  pinMode(5, INPUT_PULLDOWN);  // Player 2 right
  pinMode(19, OUTPUT);         // Game over LED
  pinMode(17, OUTPUT);
  pinMode(restart_button_pin, INPUT_PULLDOWN); // Restart button
  Serial.begin(9600);
  u8g2.begin();

  // Mode selection
  selectModes();
}


// Select game mode
void selectModes() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(10, 20);
  u8g2.print("Select Game Mode:");
  u8g2.setCursor(10, 40);
  u8g2.print("L : Snake Ventures");
  u8g2.setCursor(10, 50);
  u8g2.print("R : Ping Pong");
  u8g2.sendBuffer();

  // Wait for mode selection
  bool modeSelected = false;
  while (!modeSelected) {
    if (digitalRead(18) == HIGH) {
      game_option = PING_PONG;
      modeSelected = true;
      u8g2.clearBuffer();
      Serial.println("Ping Pong Mode Selected");
      u8g2.sendBuffer();
      delay(100);
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.setCursor(10, 20);
      u8g2.print("Select Mode:");
      u8g2.setCursor(10, 40);
      u8g2.print("L : Single Player");
      u8g2.setCursor(10, 50);
      u8g2.print("R : Dual Player");
      u8g2.setCursor(10, 60);
      u8g2.print("U : Player vs BOT");
      u8g2.sendBuffer();

    // Wait for mode selection
    bool modeSelected = false;
    while (!modeSelected) {
      if (digitalRead(16) == HIGH) {
        game_mode = SINGLE_PLAYER;
        modeSelected = true;
        Serial.println("Single Player Mode Selected");
      } else if (digitalRead(18) == HIGH) {
        game_mode = DUAL_PLAYER;
        modeSelected = true;
        Serial.println("Dual Player Mode Selected");
      } else if (digitalRead(4) == HIGH) {
        game_mode = PLAYER_VS_AI;
        modeSelected = true;
        Serial.println("Player vs BOT Mode Selected");
      }
      delay(50); // Debounce delay
  }
    }
     else if (digitalRead(16) == HIGH) {
      game_option = SNAKE_VENTURE;
      modeSelected = true;
      Serial.println("Snake Ventures Mode Selected");
    }
    delay(50); // Debounce delay
  }
    

    
  delay(500); // Prevent immediate re-triggering
}

void loop() {
  if (isButtonPressed(17)) {
    resetGame();  // Reset game state when reset button is pressed
    selectMode(); // Ask for game mode again
  }

  if (game_option == PING_PONG) {
     
     startPingPongGame();
  } else if (game_option == SNAKE_VENTURE) {
    startSnakeGame();
  }
}


// Ping Pong Game Logic
void startPingPongGame() {
  if (digitalRead(restart_button_pin) == HIGH) {
    resetGamep();  // Reset game state when restart button is pressed
    selectMode(); // Ask the user to select the game mode again
  }
  // Display scores
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(10, 10);
  u8g2.print("P1: ");
  u8g2.print(score1);

  if (game_mode == DUAL_PLAYER) { // Show Player 2 score only in relevant modes
    u8g2.setCursor(100, 10);
    u8g2.print("P2: ");
    u8g2.print(score2);
  }
  // Paddle 1 (bottom) movement
  player1_right = digitalRead(18);
  if (player1_right && paddle1_x + paddle_width <= 127) {
    paddle1_x += 3;
  }
  player1_left = digitalRead(16);
  if (player1_left && paddle1_x >= 1) {
    paddle1_x -= 3;
  }

  // Paddle 2 movement based on game mode
  if (game_mode == SINGLE_PLAYER || game_mode == PLAYER_VS_AI) {
    // AI control for Paddle 2 (top)
    if (ball_x > paddle2_x + paddle_width / 2 && paddle2_x + paddle_width <= 127) {
      paddle2_x += 2;
    } else if (ball_x < paddle2_x + paddle_width / 2 && paddle2_x >= 1) {
      paddle2_x -= 2;
    }
  } else {
    // Paddle 2 (top) movement for Player 2
    player2_right = digitalRead(5);
    if (player2_right && paddle2_x + paddle_width <= 127) {
      paddle2_x += 3;
    }
    player2_left = digitalRead(4);
    if (player2_left && paddle2_x >= 1) {
      paddle2_x -= 3;
    }
  }

  // Ball boundary conditions
  if (ball_x >= 125 || ball_x <= 2) ball_x_speed = -ball_x_speed;

  // Ball collision with Paddle 1 (bottom)
  if (ball_x >= paddle1_x && ball_x <= paddle1_x + paddle_width && ball_y + ball_radius >= paddle1_y) {
    if (!ball_hit_paddle1 && ball_y_speed > 0) { // Ensure single collision
      ball_y_speed = -ball_y_speed;
      score1++;
      digitalWrite(19, HIGH);
      delay(20);
      digitalWrite(19, LOW);
      ball_hit_paddle1 = true; // Mark collision
    }
  } else {
    ball_hit_paddle1 = false; // Reset collision flag when ball moves away
  }

  // Ball collision with Paddle 2 (top) or upper screen boundary
  if (game_mode == SINGLE_PLAYER || game_mode == PLAYER_VS_AI) {
    // Bounce off the upper screen boundary in AI modes
    if (ball_y - ball_radius <= 0) {
      ball_y_speed = -ball_y_speed;
    }
  } else {
    // Ball collision with Paddle 2 (top) in dual-player mode
    if (ball_x >= paddle2_x && ball_x <= paddle2_x + paddle_width && ball_y - ball_radius <= paddle2_y + paddle_height) {
      if (!ball_hit_paddle2 && ball_y_speed < 0) { // Ensure single collision
        ball_y_speed = -ball_y_speed;
        score2++;
        digitalWrite(19, HIGH);
        delay(20);
        digitalWrite(19, LOW);
        ball_hit_paddle2 = true; // Mark collision
      }
    } else {
      ball_hit_paddle2 = false; // Reset collision flag when ball moves away
    }
  }

  // Update ball position
  ball_x += ball_x_speed;
  ball_y += ball_y_speed;

  // Draw ball and paddles
  u8g2.drawDisc(ball_x, ball_y, ball_radius, U8G2_DRAW_ALL);
  u8g2.drawBox(paddle1_x, paddle1_y, paddle_width, paddle_height); // Player 1 paddle
  if (game_mode != SINGLE_PLAYER) {
    u8g2.drawBox(paddle2_x, paddle2_y, paddle_width, paddle_height); // Player 2 paddle (in relevant modes)
  }
  u8g2.sendBuffer();

  // Game over conditions
  if (ball_y >= Screen_ht) { // Ball passes Paddle 1
    displayFinalScore();
  } else if (ball_y <= 0 && game_mode != SINGLE_PLAYER) { // Ball passes Paddle 2
    displayFinalScore();
  }
}

void selectMode() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(10, 20);
  u8g2.print("Select Mode:");
  u8g2.setCursor(10, 40);
  u8g2.print("L : Single Player");
  u8g2.setCursor(10, 50);
  u8g2.print("R : Dual Player");
  u8g2.setCursor(10, 60);
  u8g2.print("U : Player vs BOT");
  u8g2.sendBuffer();

  // Wait for mode selection
  bool modeSelected = false;
  while (!modeSelected) {
    if (digitalRead(16) == HIGH) {
      game_mode = SINGLE_PLAYER;
      modeSelected = true;
      Serial.println("Single Player Mode Selected");
    } else if (digitalRead(18) == HIGH) {
      game_mode = DUAL_PLAYER;
      modeSelected = true;
      Serial.println("Dual Player Mode Selected");
    } else if (digitalRead(4) == HIGH) {
      game_mode = PLAYER_VS_AI;
      modeSelected = true;
      Serial.println("Player vs BOT Mode Selected");
    }
    delay(50); // Debounce delay
  }

  delay(500); // Prevent immediate re-triggering
}

void displayFinalScore() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(25, 10);
  u8g2.print("Game Over");
  u8g2.setCursor(10, 30);
  u8g2.print("Final Score of P1: ");
  u8g2.print(score1);

  if (game_mode == DUAL_PLAYER) { // Show Player 2's final score in relevant modes
    u8g2.setCursor(10, 40);
    u8g2.print("Final Score of P2: ");
    u8g2.print(score2);
    if(score1>score2)
    {
      u8g2.setCursor(10, 60);
      u8g2.print("Player 1 WINs!");
    }
    if((score2==score1) && score2!=0)
    {
      u8g2.setCursor(10, 60);
      u8g2.print("Player 2 WINs!");
    }
    if(score1==0 )
    {
      u8g2.setCursor(15, 60);
      u8g2.print("Draw :(");
    }

  }
    digitalWrite(17, HIGH);
    delay(2000);
    digitalWrite(17, LOW);

  u8g2.sendBuffer();

  // Wait for button press to restart
  waitForRestartp();
}

void waitForRestartp() {
  bool restart = false;
  while (!restart) {
    if (digitalRead(4) == HIGH || digitalRead(5) == HIGH || digitalRead(16) == HIGH || digitalRead(18) == HIGH || digitalRead(25) == HIGH) {
      restart = true;
    }
    delay(50); // Debounce delay
  }

  resetGamep();
}

void resetGamep() {
  ball_x = 64;
  ball_y = 32;
  ball_x_speed = 1;
  ball_y_speed = 1;
  score1 = 0;
  score2 = 0;
  paddle1_x = 55;
  paddle2_x = 55;
}




void startSnakeGame() {

  if (isButtonPressed(RESET_BUTTON)) {
    selectModes();  // Reset game state when restart button is pressed
  }

  if (gameOver) {
    // Display "Game Over" screen and the final score
    digitalWrite(17, HIGH);
    delay(2000);
    digitalWrite(17, LOW);
    displayGameOver();
  }

  readButtons();  // Read button input to update direction
  updateSnake();  // Move the snake
  checkCollision(); // Check for collisions
  drawGame();     // Render the game
  delay(150);     // Control snake speed
}

// Generate food at a random position
void generateFood() {
  foodX = (random(0, 128 / 4)) * 4; // Align to grid (4x4 blocks)
  foodY = (random(0, 64 / 4)) * 4;
}

// Update snake position
void updateSnake() {
  // Move the body of the snake
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  // Move the head of the snake
  snakeX[0] += dirX * 4;
  snakeY[0] += dirY * 4;

  // Check if the snake eats the food
  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    snakeLength++; // Grow the snake
    generateFood(); // Generate a new food
    digitalWrite(19, HIGH);
    delay(20);
    digitalWrite(19, LOW);
  }
}

// Check for collisions with walls or itself
void checkCollision() {
  // Check wall collision
  if (snakeX[0] < 0 || snakeX[0] >= 128 || snakeY[0] < 0 || snakeY[0] >= 64) {
    gameOver = true;
  }

  // Check self-collision
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver = true;
    }
  }
}

// Draw the game on the display
void drawGame() {
  u8g2.clearBuffer();

  // Draw the snake
  for (int i = 0; i < snakeLength; i++) {
    u8g2.drawBox(snakeX[i], snakeY[i], 4, 4); // Snake segments are 4x4 blocks
  }

  // Draw the food
  u8g2.drawBox(foodX, foodY, 4, 4);

  u8g2.sendBuffer();
}

// Read button input to change direction
void readButtons() {
  if (digitalRead(UP_BUTTON) == HIGH && dirY == 0) {
    dirX = 0; dirY = -1; // Move up
  } else if (digitalRead(DOWN_BUTTON) == HIGH && dirY == 0) {
    dirX = 0; dirY = 1; // Move down
  } else if (digitalRead(LEFT_BUTTON) == HIGH && dirX == 0) {
    dirX = -1; dirY = 0; // Move left
  } else if (digitalRead(RIGHT_BUTTON) == HIGH && dirX == 0) {
    dirX = 1; dirY = 0; // Move right
  }
}

// Reset the game (clear snake, reset length, generate new food)
void resetGame() {
  snakeLength = 5;
  dirX = 1; dirY = 0; // Reset direction to right
  gameOver = false;

  // Clear the snake's position
  for (int i = 0; i < 100; i++) {
    snakeX[i] = 64;
    snakeY[i] = 32;
  }

  // Generate new food position
  generateFood();
}

// Display the Game Over screen and show the score
void displayGameOver() {
  u8g2.clearBuffer();

  // Display "Game Over" message
  u8g2.drawStr(30, 25, "Game Over!");

  // Display the score (snake length - 5, since length starts at 5)
  String score = "Score: " + String(snakeLength - 5);
  u8g2.drawStr(30, 45, score.c_str());

  u8g2.sendBuffer();
  // Wait for button press to restart
  waitForRestart();
}

void waitForRestart() {
  bool restart = false;
  while (!restart) {
    if (digitalRead(9) == HIGH || digitalRead(18) == HIGH || digitalRead(17) == HIGH || digitalRead(16) == HIGH || digitalRead(25) == HIGH) {
      restart = true;
    }
    delay(50); // Debounce delay
  }

  resetGame();
}

// Check if the button is pressed and debounce it
bool isButtonPressed(int pin) {
  static unsigned long lastPressTime = 0;
  unsigned long currentMillis = millis();

  // Check if button is pressed
  if (digitalRead(pin) == HIGH && currentMillis - lastPressTime > 1000) { // Debounce time of 500ms
    lastPressTime = currentMillis;
    return true;
  }

  return false;
}