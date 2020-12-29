#include <iostream>

#include <thread>
#include <vector>

#include <Windows.h>

std::wstring tetromino[7];
const int field_width = 12;
const int field_height = 18;
unsigned char* field = nullptr;

const int screen_width = 80;
const int screen_height = 30;

// return rotated index
int rotate(int px, int py, int r)
{
	switch (r % 4)
	{
	case 0: return py * 4 + px;
	case 1: return 12 + py - (px * 4);
	case 2: return 15 - (py * 4) - px;
	case 3: return 3 - py + (px * 4);
	}
	return 0;
}


bool does_piece_fit(int n_tetromino, int rotation, int pos_x, int pos_y)
{
	for (int px = 0; px < 4; px++)
		for (int py = 0; py < 4; py++)
		{
			int pi = rotate(px, py, rotation);
			int fi = (pos_y + py) * field_width + (pos_x + px);

			if (pos_x + px >= 0 && pos_x + px < field_width)
			{
				if (pos_y + py >= 0 && pos_y + py < field_height)
				{
					if (tetromino[n_tetromino][pi] == L'X' && field[fi] != 0)
						return false;
				}
			}
		}
	return true;
}

int main()
{
	// assets
	tetromino[0].append(L"..X.");
	tetromino[0].append(L"..X.");
	tetromino[0].append(L"..X.");
	tetromino[0].append(L"..X.");

	tetromino[1].append(L"..X.");
	tetromino[1].append(L".XX.");
	tetromino[1].append(L".X..");
	tetromino[1].append(L"....");

	tetromino[2].append(L".X..");
	tetromino[2].append(L".XX.");
	tetromino[2].append(L"..X.");
	tetromino[2].append(L"....");

	tetromino[3].append(L"....");
	tetromino[3].append(L".XX.");
	tetromino[3].append(L".XX.");
	tetromino[3].append(L"....");

	tetromino[4].append(L"..X.");
	tetromino[4].append(L".XX.");
	tetromino[4].append(L"..X.");
	tetromino[4].append(L"....");

	tetromino[5].append(L"....");
	tetromino[5].append(L".XX.");
	tetromino[5].append(L"..X.");
	tetromino[5].append(L"..X.");

	tetromino[6].append(L"....");
	tetromino[6].append(L".XX.");
	tetromino[6].append(L".X..");
	tetromino[6].append(L".X..");

	field = new unsigned char[field_width * field_height];
	for (int x = 0; x < field_width; x++)
		for (int y = 0; y < field_height; y++)
			field[y * field_width + x] = (x == 0 || x == field_width - 1 || y == field_height - 1) ? 9 : 0;

	wchar_t* screen = new wchar_t[screen_width * screen_height];
	for (int i = 0; i < screen_width * screen_height; i++) screen[i] = L' ';
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);

	DWORD bytes_written = 0;

	bool game_over = false;

	int current_piece = 0;
	int current_rotation = 0;
	int current_x = field_width / 2;
	int current_y = 0;

	bool key[4];
	bool rotate_hold = false;

	int speed = 20;
	int speed_counter = 0;
	int force_down = false;
	int piece_count = 0;

	std::vector<int> lines;
	int score = 0;

	while (!game_over)
	{
		using namespace std::chrono_literals;

		// game
		std::this_thread::sleep_for(50ms);
		speed_counter++;

		force_down = speed == speed_counter;

		// input
		for (int k = 0; k < 4; k++) // R L D Z
			key[k] = (0x8000 & GetAsyncKeyState((unsigned char)("\x27\x25\x28Z"[k]))) != 0;

		// logic
		current_x += key[0] && does_piece_fit(current_piece, current_rotation, current_x + 1, current_y) ? 1 : 0;
		current_x -= key[1] && does_piece_fit(current_piece, current_rotation, current_x - 1, current_y) ? 1 : 0;
		current_y += key[2] && does_piece_fit(current_piece, current_rotation, current_x, current_y + 1) ? 1 : 0;
		if (key[3])
		{
			current_rotation += !rotate_hold && does_piece_fit(current_piece, current_rotation + 1, current_x, current_y) ? 1 : 0;
			rotate_hold = true;
		}
		else
		{
			rotate_hold = false;
		}

		if (force_down)
		{
			piece_count++;
			if (piece_count % 50 == 0)
				if (speed >= 10) speed--;

			if (does_piece_fit(current_piece, current_rotation, current_x, current_y + 1))
				current_y++;
			else
			{
				// lock current piece in the field
				for (int px = 0; px < 4; px++)
					for (int py = 0; py < 4; py++)
						if (tetromino[current_piece][rotate(px, py, current_rotation)] == L'X')
							field[(current_y + py) * field_width + (current_x + px)] = current_piece + 1;

				// check have we got any lines
				for (int py = 0; py < 4; py++)
					if (current_y + py < field_height - 1)
					{
						bool line = true;
						for (int px = 1; px < field_width - 1; px++)
							line &= (field[(current_y + py) * field_width + px]) != 0;

						if (line)
						{
							for (int px = 1; px < field_width - 1; px++)
								field[(current_y + py) * field_width + px] = 8;

							lines.push_back(current_y + py);
						}
					}

				score += 25;
				if (!lines.empty())	score += (1 << lines.size()) * 100;

				// choose next piece
				current_x = field_width / 2;
				current_y = 0;
				current_rotation = 0;
				current_piece = rand() % 7;

				// piece does not fit
				game_over = !does_piece_fit(current_piece, current_rotation, current_x, current_y);

			}

			speed_counter = 0;
		}

		// draw field
		for (int x = 0; x < field_width; x++)
			for (int y = 0; y < field_height; y++) // by field values, draw specific character. 0 -> " " / 9 -> "#"
				screen[(y + 2) * screen_width + (x + 2)] = L" ABCDEFG=#"[field[y * field_width + x]];

		for (int px = 0; px < 4; px++)
			for (int py = 0; py < 4; py++)
				if (tetromino[current_piece][rotate(px, py, current_rotation)] == L'X')
					screen[(current_y + py + 2) * screen_width + (current_x + px + 2)] = current_piece + 65;

		// Draw Score
		swprintf_s(&screen[2 * screen_width + field_width + 6], 16, L"SCORE: %8d", score);

		if (!lines.empty())
		{
			// display frame
			WriteConsoleOutputCharacter(hConsole, screen, screen_width * screen_height, { 0,0 }, &bytes_written);
			std::this_thread::sleep_for(400ms);

			for (auto& l : lines)
				for (int px = 1; px < field_width - 1; px++)
				{
					for (int py = l; py > 0; py--)
						field[py * field_width + px] = field[(py - 1) * field_width + px];
					field[px] = 0;
				}

			lines.clear();
		}
		// display frame
		WriteConsoleOutputCharacter(hConsole, screen, screen_width * screen_height, { 0,0 }, &bytes_written);
	}

	CloseHandle(hConsole);
	std::cout << "Game Over!! Score:" << score << std::endl;
	system("pause");
	return 0;
}