#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>

using namespace std;

#include <stdio.h>
#include <Windows.h>

int nScreenWidth = 120; // Console Screen Size X (columns)
int nScreenHeight = 40; // Console Screen Size Y (rows)
 
float fPlayerX = 8.0f;  // Player Start Position
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;	// Player's angle he is looking at

int nMapWidth = 16;    // Map 
int nMapHeight = 16; 

float fDepth = 16.0f;			// Maximum rendering distance
float fFOV = 3.14159 / 4.0f; // is an agle 


int main()
{
	// Create Screen Buffer
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	DWORD dwBytesWritten = 0;

	// Create Map of world space # = wall block, . = space
	wstring map;
 	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.....#........#";
	map += L"#.....#........#";
	map += L"#.....#........#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.........##...#";
	map += L"#.........##...#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";

	auto tp1 = chrono::system_clock::now(); // Переменные для подсчета

	auto tp2 = chrono::system_clock::now(); // пройденного времени

	while (1) {
		tp2 = chrono::system_clock::now();
		chrono::duration <float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (1.5f) * fElapsedTime; // Клавишей "A" поворачиваем по часовой стрелке
 		 
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (1.5f) * fElapsedTime; // Клавишей "D" поворачиваем против часовой стрелки

		if (GetAsyncKeyState((unsigned short)'W') & 0x8000) { // Клавишей "W" идём вперёд
			fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') { // Если столкнулись со стеной, но откатываем шаг

				fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
			}

		}

		if (GetAsyncKeyState((unsigned short)'S') & 0x8000) { // Клавишей "S" идём назад
			fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;

			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') { // Если столкнулись со стеной, но откатываем шаг
				fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		for (int x = 0; x < nScreenWidth; ++x) {
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

			float fStepSize = 0.1f;		   // Increment size for ray casting, decrease to increase resolution	
			float fDistanceToWall = 0.0f;  // Расстояние до препятствия в направлении fRayAngle
			float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			bool bHitWall = false;		   // Достигнул ли луч стенку
			bool bBoundary = false;

			while (!bHitWall && fDistanceToWall < fDepth) // Пока не столкнулись со стеной
			{
				fDistanceToWall += fStepSize;
				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{ // Если мы вышли за зону
					bHitWall = true;
					fDistanceToWall = fDepth; // Just set distance to maximum depth
				}
				// // Ray is inbounds so test to see if the ray cell is a wall block
				else if (map[nTestY * nMapWidth + nTestX] == '#')
				{
					bHitWall = true;

					vector<pair<float, float>> p; // distance, dot

					for (int tx = 0; tx < 2; tx++)
						for (int ty = 0; ty < 2; ty++)
						{
							// Angle of corner to eye
							float vy = (float)nTestY + ty - fPlayerY;
							float vx = (float)nTestX + tx - fPlayerX;
							float d = sqrt(vx * vx + vy * vy);
							float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
							p.push_back(make_pair(d, dot));
						}

					// Sort Pairs from closest to farthest
					sort(p.begin(), p.end(), [](const pair<float, float>& left, const pair<float, float>& right) {return left.first < right.first; });

					// First two/three are closest (we will never see all four)
					float fBound = 0.01;
					if (acos(p.at(0).second) < fBound) bBoundary = true;
					if (acos(p.at(1).second) < fBound) bBoundary = true;
					if (acos(p.at(2).second) < fBound) bBoundary = true;
				}
			}
			// Calculate distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';
			if (fDistanceToWall <= fDepth / 4.0f)			nShade = 0x2588;	// Very close	
			else if (fDistanceToWall < fDepth / 3.0f)		nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)		nShade = 0x2592;
			else if (fDistanceToWall < fDepth)				nShade = 0x2591;
			else											nShade = ' ';		// Too far away

			if (bBoundary) nShade = ' ';

			for (int y = 0; y < nScreenHeight; ++y) {
				// Each Row -> it's ceiling 
				if (y <= nCeiling)
					screen[y * nScreenWidth + x] = ' ';
				else if(y>nCeiling && y <= nFloor) // wall
					screen[y * nScreenWidth + x] = nShade ;
				else // floor 
				{
					// Shade floor based on distance
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)		nShade = '#';
					else if (b < 0.5)	nShade = 'x';
					else if (b < 0.75)	nShade = '.';
					else if (b < 0.9)	nShade = '-';
					else				nShade = ' ';
					screen[y * nScreenWidth + x] = nShade;
				}
					
			}
		}
		
		// Display Stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

		// Display Map
		for (int nx = 0; nx < nMapWidth; ++nx)
			for (int ny = 0; ny < nMapWidth; ++ny)
				screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];

		screen[((int)fPlayerX + 1) * nScreenWidth + (int)fPlayerY] = 'P';

		screen[nScreenWidth * nScreenHeight - 1] = '\0'; // to know where to stop
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}
}

