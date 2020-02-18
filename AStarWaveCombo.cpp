#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <utility>
//! LEFT OFF 32:32
using namespace std;
sf::Vector2u WINDOW_SIZE{800,480};


/* Background
~~~~~~~~~~
A nice follow up alternative to the A* Algorithm. Wave propagation is less
applicable to multiple objects with multiple destinations, but fantatsic
for multiple objects all reaching the same destination.*/
struct WavePropagation {

	WavePropagation() {
		nBorderWidth = 4;
		nCellSize = 32;
		nMapWidth = WINDOW_SIZE.x / nCellSize;
		nMapHeight = WINDOW_SIZE.y / nCellSize;

		nStartX = 3;
		nStartY = 7;

		nEndX = 12;
		nEndY = 7;

		nWave = 1;

		bEightConnectivity = false;
		bShowText = false;
		bShowArrows = false;

		bObstacleMap = new bool[nMapWidth * nMapHeight]{false};
		nFlowFieldZ = new int[nMapWidth * nMapHeight]{0};
		fFlowFieldX = new float[nMapWidth * nMapHeight]{0};
		fFlowFieldY = new float[nMapWidth * nMapHeight]{0};

		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {
				sfShape.setFillColor(sf::Color::Blue);
				sfShape.setSize(sf::Vector2f{(float)nCellSize - nBorderWidth,(float)nCellSize - nBorderWidth});
			}
		}

		sfFont.loadFromFile("assets/fonts/Amita-Bold.ttf");
		sfText.setFont(sfFont);
		sfText.setCharacterSize(14);
		sfText.setFillColor(sf::Color::White);
		sfText.setString(" ");
		sfText.setStyle(sf::Text::Style::Bold);

	}

	~WavePropagation() {
		delete[] bObstacleMap;
		delete[] nFlowFieldZ;

	}

	float d = 0.f;

	bool onUserUpdate(sf::RenderWindow* window, float dt) {

		// little convience lambda 2D -> 1D
		auto p = [&](int x, int y) { return y * nMapWidth + x; };

		// user input
		sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);

		int nSelectedCellX = sfMousePos.x / nCellSize;
		int nSelectedCellY = sfMousePos.y / nCellSize;

		d += dt;

		if(d > 0.325)
		{
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				// toggle obstacal if left mouse is clicked
				bObstacleMap[p(nSelectedCellX, nSelectedCellY)] = !bObstacleMap[p(nSelectedCellX, nSelectedCellY)];
			}


			// Start Goal (Right Click)
			if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
				nStartX = nSelectedCellX;
				nStartY = nSelectedCellY;
			}

			// End Goal (Left Control + Right Click)
			if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
				nEndX = nSelectedCellX;
				nEndY = nSelectedCellY;
			}
		}

		// resets and create a border wall of obstacales
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			for (auto x = 0; x < nMapWidth; x++)
				for (auto y = 0; y < nMapHeight; y++) {
					bObstacleMap[p(x, y)] = false;
					if (x == 0 || y == 0 || x == nMapWidth - 1 || y == nMapHeight - 1)
						bObstacleMap[p(x, y)] = !bObstacleMap[p(x, y)];
				}
		}

		// toggle 8 way connectivity
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
			bEightConnectivity = !bEightConnectivity;
		}

		// toggles text on blocks
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)) {
			bShowText = !bShowText;
		}

		// toggles text on blocks
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			bShowArrows = !bShowArrows;
		}

		// clear all blocks
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
			for (auto x = 0; x < nMapWidth; x++)
				for (auto y = 0; y < nMapHeight; y++) {
					bObstacleMap[p(x, y)] = false;
				}
		}


		// propagate the wave to see the path choosen Q/A
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
			++nWave;
			if (nWave > nMapWidth - 1)
				nWave = nMapWidth - 1;
			std::cout << nWave << "\n";
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) {
			--nWave;
			if (nWave == 0)
				nWave = 1;
		}
		// 1) Prepare the flow field, add a boundary, and add obstacles by setting the flow field height (Z) to -1
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {
				// Set border or obstacles 
				if (x == 0 || y == 0 || x == (nMapWidth - 1) || y == (nMapHeight - 1) || bObstacleMap[p(x, y)]) {
					nFlowFieldZ[p(x, y)] = -1;
				}
				else {
					nFlowFieldZ[p(x, y)] = 0;
				}
			}
		}

		// Propagate a wave (i.e. flood-fill) from target locations. We use a tuple of {x, y, distance} - though you could use a struct or similar
		std::list<std::tuple<int, int, int>> nodes(0);

		// Add the first discovered node - the target location, with a distance of 1
		nodes.emplace_back(nEndX, nEndY, 1);

		while (!nodes.empty()) {
			// Each node through the discovered nodes may create newly discovered nodes, so I maintain a second list. 
			// It's important not to contaminate the list being iterated through.
			std::list<std::tuple<int, int, int>> newNodes;

			// Now iterate through each discovered node. 
			// If it has neighboring nodes that are empty space and undiscovered, add those locations to the new nodes list.
			for (auto& n : nodes) {

				int x = std::get<0>(n); // Map X-Coordinate
				int y = std::get<1>(n); // Map Y-Coordinate
				int d = std::get<2>(n); // Distance From Target Location

				// Set the distance count for this node. Note: That when we add nodes we add 1 to the distance. 
				// This emulates propagating a wave across the map, where the front of that wave increments each iteration.
				// In this way, we can propagate distance information 'away' from target location'
				nFlowFieldZ[p(x, y)] = d;

				// Add neighbor nodes if unmarked, i.e. their "height" is 0. Any discovered nodes or obstacle will be non-zero

				// Check East
				if ((x + 1) < nMapWidth && nFlowFieldZ[p(x + 1, y)] == 0)
					newNodes.emplace_back(x + 1, y, d + 1);

				// Check West
				if ((x - 1) >= 0 && nFlowFieldZ[p(x - 1, y)] == 0)
					newNodes.emplace_back(x - 1, y, d + 1);

				// Check South
				if ((y + 1) < nMapHeight && nFlowFieldZ[p(x, y + 1)] == 0)
					newNodes.emplace_back(x, y + 1, d + 1);

				// Check North
				if ((y - 1) >= 0 && nFlowFieldZ[p(x, y - 1)] == 0)
					newNodes.emplace_back(x, y - 1, d + 1);
			}



			// We will now have potentially multiple nodes for a single location. This means our algorithm will never complete!
			// So we must remove duplicates form our new node list. 
			// Note: Do away with overhead structures like linked list and sorts for fastest path-finding

			// Sort the nodes - This will stack up nodes that are similar: A, B, B, B, B, C, D, D, E, F, F
			newNodes.sort([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
						  {
							  // In this instance, you don't care how the values are sorted, so long as nodes that represent the same location are adjacent in the list.
							  // Remember to use the lambda to get a 1D value for a 2D coordinate.
							  return p(std::get<0>(n1), std::get<1>(n1)) < p(std::get<0>(n2), std::get<1>(n2));
						  });
			//														   -, -, -,       -,       -,
			// Use "unique" function to remove adjacent duplicates		: A, B, B, B, B, C, D, D, E, F, F
			// and also erase them									: A, B, C, D, E, F
			newNodes.unique([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
							{
								return  p(std::get<0>(n1), std::get<1>(n1)) == p(std::get<0>(n2), std::get<1>(n2));
							});

			// We've now processed all the discovered nodes, so clear the list, and add the newly discovered nodes for processing on the next iteration
			nodes.clear();
			nodes.insert(nodes.begin(), newNodes.begin(), newNodes.end());

			// When there are no more newly discovered nodes, we have "flood filled" the entire map. The propagation phase of the algorithm is complete
		}

		// 3) Create path. Starting a start location, create a path of nodes until you reach target location.
		// At each node find the neighbor with the lowest "D" (distance) score.
		std::list<std::pair<int, int>> path;
		path.emplace_back(nStartX, nStartY);
		int nLocX = nStartX;
		int nLocY = nStartY;
		bool bNoPath = false;

		while (!(nLocX == nEndX && nLocY == nEndY) && !bNoPath) {
			// for each neighbor generate another list of tuples
			std::list<std::tuple<int, int, int>> listNeighbors;

			// 4-Way connectivity
			// North
			if ((nLocY - 1) >= 0 && nFlowFieldZ[p(nLocX, nLocY - 1)] > 0)
				listNeighbors.emplace_back(nLocX, nLocY - 1, nFlowFieldZ[p(nLocX, nLocY - 1)]);
			// East
			if ((nLocX + 1) >= 0 && nFlowFieldZ[p(nLocX + 1, nLocY)] > 0)
				listNeighbors.emplace_back(nLocX + 1, nLocY, nFlowFieldZ[p(nLocX + 1, nLocY)]);
			// South
			if ((nLocY + 1) >= 0 && nFlowFieldZ[p(nLocX, nLocY + 1)] > 0)
				listNeighbors.emplace_back(nLocX, nLocY + 1, nFlowFieldZ[p(nLocX, nLocY + 1)]);
			// West
			if ((nLocX - 1) >= 0 && nFlowFieldZ[p(nLocX - 1, nLocY)] > 0)
				listNeighbors.emplace_back(nLocX - 1, nLocY, nFlowFieldZ[p(nLocX - 1, nLocY)]);

			// 8-Way connectivity
			if (bEightConnectivity) {
				if ((nLocY - 1) >= 0 && (nLocX - 1) >= 0 && nFlowFieldZ[p(nLocX - 1, nLocY - 1)] > 0)
					listNeighbors.emplace_back(nLocX - 1, nLocY - 1, nFlowFieldZ[p(nLocX - 1, nLocY - 1)]);

				if ((nLocY - 1) >= 0 && (nLocX + 1) < nMapWidth && nFlowFieldZ[p(nLocX + 1, nLocY - 1)] > 0)
					listNeighbors.emplace_back(nLocX + 1, nLocY - 1, nFlowFieldZ[p(nLocX + 1, nLocY - 1)]);

				if ((nLocY + 1) < nMapHeight && (nLocX - 1) >= 0 && nFlowFieldZ[p(nLocX - 1, nLocY + 1)] > 0)
					listNeighbors.emplace_back(nLocX - 1, nLocY + 1, nFlowFieldZ[p(nLocX - 1, nLocY + 1)]);

				if ((nLocY + 1) < nMapHeight && (nLocX + 1) < nMapWidth && nFlowFieldZ[p(nLocX + 1, nLocY + 1)] > 0)
					listNeighbors.emplace_back(nLocX + 1, nLocY + 1, nFlowFieldZ[p(nLocX + 1, nLocY + 1)]);
			}
			// Sprt neigbours based on height, so lowest neighbour is at front
			// of list
			listNeighbors.sort([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
							   {
								   return std::get<2>(n1) < std::get<2>(n2); // Compare distances
							   });

			if (listNeighbors.empty()) // Neighbour is invalid or no possible path
				bNoPath = true;
			else {
				nLocX = std::get<0>(listNeighbors.front());
				nLocY = std::get<1>(listNeighbors.front());
				path.push_back({nLocX, nLocY});
			}
		}

		// 4) Create Flow "Field"
		for (auto x = 0; x < nMapWidth - 1; x++)
			for (auto y = 0; y < nMapHeight - 1; y++) {

				float vx = 0.f;
				float vy = 0.f;

				vy -= static_cast<float>((nFlowFieldZ[p(x, y + 1)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x, y + 1)]) - nFlowFieldZ[p(x, y)]);
				vx -= static_cast<float>((nFlowFieldZ[p(x + 1, y)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x + 1, y)]) - nFlowFieldZ[p(x, y)]);
				vy += static_cast<float>((nFlowFieldZ[p(x, y - 1)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x, y - 1)]) - nFlowFieldZ[p(x, y)]);
				vx += static_cast<float>((nFlowFieldZ[p(x - 1, y)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x - 1, y)]) - nFlowFieldZ[p(x, y)]);

				float r = 1.0f / sqrtf(vx * vx + vy * vy);
				fFlowFieldX[p(x, y)] = vx * r;
				fFlowFieldY[p(x, y)] = vy * r;
			}

		int ob = 0;
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {

				if (bObstacleMap[p(x, y)]) {
					sfShape.setFillColor(sf::Color{170,170,170,255});
					++ob;
				}
				else
					sfShape.setFillColor(sf::Color::Blue);

				// shows the wave that is propagated
				if (nWave == nFlowFieldZ[p(x, y)])
					sfShape.setFillColor(sf::Color::Cyan);

				if (x == nStartX && y == nStartY)
					sfShape.setFillColor(sf::Color::Green);

				if (x == nEndX && y == nEndY)
					sfShape.setFillColor(sf::Color::Red);


				sfShape.setPosition(x * nCellSize - nBorderWidth, y * nCellSize - nBorderWidth);
				// Draw Base
				window->draw(sfShape);

				// Draw "potential" or "distance" or "height" 
				sfText.setString(std::to_string(nFlowFieldZ[p(x, y)]));
				sfText.setOrigin(sfText.getCharacterSize() / 2, sfText.getCharacterSize() / 2 - 1);
				sfText.setPosition(x * nCellSize + nBorderWidth, y * nCellSize + 1);

				// draw text on blocks
				if (bShowText)
					window->draw(sfText);

				// Drawing Arrow
				if (nFlowFieldZ[p(x, y)] > 0) {
					float ax[4], ay[4];

					float fAngle = atan2f(fFlowFieldY[p(x, y)], fFlowFieldX[p(x, y)]);
					float fRadius = static_cast<float>(nCellSize - nBorderWidth) / 2.f;

					int offsetX = x * nCellSize + ((nCellSize - nBorderWidth) / 2);
					int offsetY = y * nCellSize + ((nCellSize - nBorderWidth) / 2);

					ax[0] = cosf(fAngle) * fRadius + offsetX;
					ay[0] = sinf(fAngle) * fRadius + offsetY;

					ax[1] = cosf(fAngle) * -fRadius + offsetX;
					ay[1] = sinf(fAngle) * -fRadius + offsetY;

					ax[2] = cosf(fAngle + 0.1f) * fRadius * 0.7f + offsetX;
					ay[2] = sinf(fAngle + 0.1f) * fRadius * 0.7f + offsetY;

					ax[3] = cosf(fAngle - 0.1f) * fRadius * 0.7f + offsetX;
					ay[3] = sinf(fAngle - 0.1f) * fRadius * 0.7f + offsetY;

					sf::Vertex arrow[] =
					{
						sf::Vertex(sf::Vector2f(ax[0],ay[0])),
						sf::Vertex(sf::Vector2f(ax[1],ay[1])),

						sf::Vertex(sf::Vector2f(ax[0],ay[0])),
						sf::Vertex(sf::Vector2f(ax[2],ay[2])),

						sf::Vertex(sf::Vector2f(ax[0],ay[0])),
						sf::Vertex(sf::Vector2f(ax[3],ay[3]))
					};

					// draw arrows
					if (bShowArrows)
						window->draw(arrow, 6, sf::Lines);
				}
			}
		}

		// extract first point seperately for visual appeasing
		bool bFirstPoint = true;
		int ox, oy;

		for (auto& a : path) {
			if (bFirstPoint) {
				// access pairs
				ox = a.first;
				oy = a.second;
				bFirstPoint = false;
			}
			else {

				sf::Vertex line[] =
				{
					sf::Vertex(sf::Vector2f(static_cast<float>(ox) * nCellSize + ((nCellSize - nBorderWidth) / 2),
													  static_cast<float>(oy) * nCellSize + ((nCellSize - nBorderWidth) / 2))),
					sf::Vertex(sf::Vector2f(static_cast<float>(a.first) * nCellSize + ((nCellSize - nBorderWidth) / 2),
													  static_cast<float>(a.second) * nCellSize + ((nCellSize - nBorderWidth) / 2)))
				};

				line->color = sf::Color::Yellow;

				window->draw(line, 2, sf::Lines);

				ox = a.first;
				oy = a.second;

				sf::CircleShape circle(9);
				circle.setOrigin(11, 9);		//? manually set
				circle.setPosition(static_cast<float>(ox) * nCellSize + ((nCellSize - nBorderWidth) / 2),
								   static_cast<float>(oy) * nCellSize + ((nCellSize - nBorderWidth) / 2));
				circle.setFillColor(sf::Color::Yellow);


				window->draw(circle);


				auto title = "[W]all Reset [" + std::to_string(ob) + "]    [C]lear    [=]/[-] WaveLength" +
					"    [8]-Connect:" + std::to_string(static_cast<int>(bEightConnectivity)) +
					"    Show[T]ext:" + std::to_string(static_cast<int>(bShowText)) +
					"    Show[A]rrows:" + std::to_string(static_cast<int>(bShowArrows));
				window->setTitle(title);
			}
		}

		return true;
	}
	int nMapWidth;
	int nMapHeight;
	int nCellSize;
	int nBorderWidth;
	sf::RectangleShape sfShape;
	bool* bObstacleMap;

	int nStartX;
	int nEndX;

	int nStartY;
	int nEndY;

	int* nFlowFieldZ;		// represents 'D' value
	float* fFlowFieldX;
	float* fFlowFieldY;

	int nWave;
	bool bEightConnectivity;
	bool bShowText;
	bool bShowArrows;

	sf::Font sfFont;
	sf::Text sfText;
};

/* Background
~~~~~~~~~~
The A* path finding algorithm is a widely used and powerful shortest path
finding node traversal algorithm. A heuristic is used to bias the algorithm
towards success. This code is probably more interesting than the video. :-/*/
struct AStar {
public:
	AStar(sf::RenderWindow* window) : window{window} {

		this->shape.setSize(sf::Vector2f{(float)this->nNodeSize - this->nNodeBorder,(float)this->nNodeSize - this->nNodeBorder});

		// Create a 2D array of nodes - this is for convenience of rendering and construction
		// and is not required for the algorithm to work - the nodes could be placed anywhere
		// in any space, in multiple dimensions...
		nodes = new sNode[nMapWidth * nMapHeight];

		for (auto x = 0; x < nMapWidth; x++) {
			for (auto y = 0; y < nMapHeight; y++) {
				nodes[y * nMapWidth + x].x = x;		// ...because we give each node its own coordinates
				nodes[y * nMapWidth + x].y = y;
				nodes[y * nMapWidth + x].bObstacle = false;
				nodes[y * nMapWidth + x].parent = nullptr;
				nodes[y * nMapWidth + x].bVisited = false;

			}
		}

		// Create connections - in this case nodes are on a regular grid
		for (auto x = 0; x < nMapWidth; x++) {
			for (auto y = 0; y < nMapHeight; y++) {
				if (y > 0)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y - 1) * nMapWidth + (x + 0)]);
				if (y < nMapHeight - 1)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 1) * nMapWidth + (x + 0)]);
				if (x > 0)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 0) * nMapWidth + (x - 1)]);
				if (x < nMapWidth - 1)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 0) * nMapWidth + (x + 1)]);

				// if diagnals are included
				if (b8Connection) {
					// We can also connect diagonally
					if (y > 0 && x > 0)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y - 1) * nMapWidth + (x - 1)]);
					if (y < nMapHeight - 1 && x>0)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 1) * nMapWidth + (x - 1)]);
					if (y > 0 && x < nMapWidth - 1)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y - 1) * nMapWidth + (x + 1)]);
					if (y < nMapHeight - 1 && x < nMapWidth - 1)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 1) * nMapWidth + (x + 1)]);
				}
			}
		}
		// Manually position the start and end markers so they are not nullptr
		nodeStart = &nodes[(nMapHeight / 2) * nMapWidth + 1];
		nodeEnd = &nodes[(nMapHeight / 2) * nMapWidth + nMapWidth - 2];

	}

	void toggleDiagnols() {

		for (auto x = 0; x < nMapWidth; x++) {
			for (auto y = 0; y < nMapHeight; y++) {
				nodes[y * nMapWidth + x].x = x;		// ...because we give each node its own coordinates
				nodes[y * nMapWidth + x].y = y;
				nodes[y * nMapWidth + x].bObstacle = nodes[y * nMapWidth + x].bObstacle;
				nodes[y * nMapWidth + x].parent = nullptr;
				nodes[y * nMapWidth + x].bVisited = false;
				nodes[y * nMapWidth + x].vecNeighbors.clear();

			}
		}
		// Create connections - in this case nodes are on a regular grid
		for (auto x = 0; x < nMapWidth; x++) {
			for (auto y = 0; y < nMapHeight; y++) {
				if (y > 0)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y - 1) * nMapWidth + (x + 0)]);
				if (y < nMapHeight - 1)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 1) * nMapWidth + (x + 0)]);
				if (x > 0)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 0) * nMapWidth + (x - 1)]);
				if (x < nMapWidth - 1)
					nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 0) * nMapWidth + (x + 1)]);

				// if diagnals are included
				if (b8Connection) {
					// We can also connect diagonally
					if (y > 0 && x > 0)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y - 1) * nMapWidth + (x - 1)]);
					if (y < nMapHeight - 1 && x>0)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 1) * nMapWidth + (x - 1)]);
					if (y > 0 && x < nMapWidth - 1)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y - 1) * nMapWidth + (x + 1)]);
					if (y < nMapHeight - 1 && x < nMapWidth - 1)
						nodes[y * nMapWidth + x].vecNeighbors.push_back(&nodes[(y + 1) * nMapWidth + (x + 1)]);
				}
			}
		}
		SolveAStar();
	}

	bool SolveAStar() {
		// reset navigation graph - default all node states
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {
				nodes[y * nMapWidth + x].bVisited = false;
				nodes[y * nMapWidth + x].fGlobalGoal = INFINITY;
				nodes[y * nMapWidth + x].fLocalGoal = INFINITY;
				nodes[y * nMapWidth + x].parent = nullptr;			// No Parents

				if (&nodes[y * nMapWidth + x] == nodeStart) {
					shape.setPosition(x * this->nNodeSize + (float)this->nNodeBorder, y * this->nNodeSize + (float)this->nNodeBorder);
					shape.setFillColor(sf::Color{127,255,0,255});
				}

				if (&nodes[y * nMapWidth + x] == nodeEnd) {
					shape.setPosition(x * this->nNodeSize + (float)this->nNodeBorder, y * this->nNodeSize + (float)this->nNodeBorder);
					shape.setFillColor(sf::Color{220,20,60,255});
				}
			}
		}

		auto distance = [](sNode* a, sNode* b)	// for convenience
		{
			return sqrt((a->x - b->x) * (a->x - b->x) + (a->y - b->y) * (a->y - b->y));
		};

		auto heuristic = [distance](sNode* a, sNode* b)		// so we can experiment with heuristic 
		{
			return distance(a, b);
		};

		// setup starting conditions
		sNode* nodeCurrent = nodeStart;
		nodeStart->fLocalGoal = 0.f;
		nodeStart->fGlobalGoal = heuristic(nodeStart, nodeEnd);

		// Add start node to not tested list - this will ensure it gets tested.
		// As the algorithm progress, newly discovered nodes get added to this list, and will themselves be tested later
		list<sNode*> listNotTestedNodes;
		listNotTestedNodes.push_back(nodeStart);

		// If the not tested list contains nodes, there may be better paths which not yet been explored.
		// However, we will also stop searching when we reach the target - there may well be better paths but this one will do - it won't be the longest
		while (!listNotTestedNodes.empty() && nodeCurrent != nodeEnd)	// find absolutely shorest path // && nodeCurrent != nodeEnd)
		{
			// Sort untested nodes by global goal, so lowest it first
			listNotTestedNodes.sort([](const sNode* lhs, const sNode* rhs) { return lhs->fGlobalGoal < rhs->fGlobalGoal; });

			// Front of listedNotTestedNodes is potentially the lowest distance node.
			// Our list may also contain nodes that have been visited, so ditch these...
			while (!listNotTestedNodes.empty() && listNotTestedNodes.front()->bVisited)
				listNotTestedNodes.pop_front();

			// ...or abort because there are no valid nodes left to test
			if (listNotTestedNodes.empty())
				break;

			nodeCurrent = listNotTestedNodes.front();
			nodeCurrent->bVisited = true;	// We only explore a node once

			// Check each of this node's neighbors...
			for (auto nodeNeighbor : nodeCurrent->vecNeighbors) {
				// ... and only if the neighbor is not visted and is not an obstacle, add it to NotTested List
				if (!nodeNeighbor->bVisited && nodeNeighbor->bObstacle == 0)
					listNotTestedNodes.push_back(nodeNeighbor);

				// Calculate the neighbors potential lowest parent distance
				float fPossiblyLowerGoal = nodeCurrent->fLocalGoal + distance(nodeCurrent, nodeNeighbor);

				// If choosing to path through this node is a lower distance than what the neighbor currently has set, update the neighbor to use this node
				// as the path source, and set its distance scores as necessary
				if (fPossiblyLowerGoal < nodeNeighbor->fLocalGoal) {
					nodeNeighbor->parent = nodeCurrent;
					nodeNeighbor->fLocalGoal = fPossiblyLowerGoal;

					// The best path length to the neighbor being tested has changed, so update the neighbor's score.
					// The heuristic is used to globally bias the path algorithm, so its knows if it's getter better or worse.
					// At some point the algorithm will realize this path is worse and abandon it, and then we go and search along the next best path.
					nodeNeighbor->fGlobalGoal = nodeNeighbor->fLocalGoal + heuristic(nodeNeighbor, nodeEnd);
				}
			}
		}
		return true;
	}

	float d = 0.f;
	void onUpdate(float dt, sf::Vector2i mousePos) {
		d += dt;
		this->mousePos = mousePos;
		// Use integer division to nicely get cursor position in node space
		int nSelectedNodeX = mousePos.x / nNodeSize;
		int nSelectedNodeY = mousePos.y / nNodeSize;

		if (d >= 0.355)
			// use mouse to draw maze, shift and ctrl to place start and end
		{
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				if (nSelectedNodeX >= 0 && nSelectedNodeX < nMapWidth) {
					if (nSelectedNodeY >= 0 && nSelectedNodeY < nMapHeight) {
						if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
							nodeStart = &nodes[nSelectedNodeY * nMapWidth + nSelectedNodeX];
						else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
							nodeEnd = &nodes[nSelectedNodeY * nMapWidth + nSelectedNodeX];
						else
							nodes[nSelectedNodeY * nMapWidth + nSelectedNodeX].bObstacle = !nodes[nSelectedNodeY * nMapWidth + nSelectedNodeX].bObstacle;

						SolveAStar();	// Solve in "real-time" gives a nice effect.
					}
				}
			}
			d = 0;
		}

		// toggles text on blocks
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
			b8Connection = !b8Connection;
			toggleDiagnols();
			this->window->setTitle("[8]Connectivity: " + std::to_string(b8Connection));
		}

		// Draw Connection First - lines from this nodes position to its connected neighbor node positions
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {
				for (auto n : nodes[y * nMapWidth + x].vecNeighbors) {
					sf::Vertex line[] = {
						sf::Vertex(sf::Vector2f(x * this->nNodeSize + this->nNodeSize / 2 + 3,
														  y * this->nNodeSize + this->nNodeSize / 2 + 3)),
						sf::Vertex(sf::Vector2f(n->x * this->nNodeSize + this->nNodeSize / 2 + 3,
														  n->y * this->nNodeSize + this->nNodeSize / 2 + 3))
					};

					line->color = sf::Color{30,144,255,255};
					this->window->draw(line, 2, sf::Lines);
				}
			}
		}


		// Draw Nodes on top
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {

				if (nodes[y * nMapWidth + x].bObstacle)
					shape.setFillColor(sf::Color{47,79,79,255});
				else
					shape.setFillColor(sf::Color{0,191,255,255});
				//	shape.setFillColor(sf::Color{0,191,255});

				shape.setPosition(x * this->nNodeSize + this->nNodeBorder, y * this->nNodeSize + this->nNodeBorder);


				if (nodes[y * nMapWidth + x].bVisited) {
					shape.setPosition(x * this->nNodeSize + (float)this->nNodeBorder, y * this->nNodeSize + (float)this->nNodeBorder);
					shape.setFillColor(sf::Color{30,144,255,255});
					//shape.setFillColor(sf::Color{0,191,255});
				}

				if (&nodes[y * nMapWidth + x] == nodeStart) {
					shape.setPosition(x * this->nNodeSize + (float)this->nNodeBorder, y * this->nNodeSize + (float)this->nNodeBorder);
					shape.setFillColor(sf::Color{127,255,0,255});
				}

				if (&nodes[y * nMapWidth + x] == nodeEnd) {
					shape.setPosition(x * this->nNodeSize + (float)this->nNodeBorder, y * this->nNodeSize + (float)this->nNodeBorder);
					shape.setFillColor(sf::Color{220,20,60,255});
				}

				this->window->draw(shape);
			}


			// Draw Path by starting ath the end, and following the parent node trail
			// back to the start - the start node will not have a parent path to follow
			if (nodeEnd != nullptr) {
				sNode* p = nodeEnd;
				while (p->parent != nullptr) {
					sf::Vertex line[]
					{
						sf::Vertex(sf::Vector2f(p->x * this->nNodeSize + this->nNodeSize / 2 + 3, p->y * this->nNodeSize + this->nNodeSize / 2 + 3)),
						sf::Vertex(sf::Vector2f(p->parent->x * this->nNodeSize + this->nNodeSize / 2 + 3, p->parent->y * this->nNodeSize + this->nNodeSize / 2 + 3))
					};
					line->color = sf::Color{255,218,185,255};

					this->window->draw(line, 2, sf::Lines);

					// Set next node to this node's parent
					p = p->parent;
				}
			}

		}
	}

private:
	struct sNode {
		bool bObstacle = false;		// is the node an obstruction
		bool bVisited = false;		// have we searched this node before?
		float fGlobalGoal;			// Distance to goal so far
		float fLocalGoal;			// Distance to goal if we took the alternative route 
		int x;						// Node position in 2D space
		int y;
		std::vector<sNode*> vecNeighbors;		// Connection to neighbors
		sNode* parent;							// Node connecting to this node that offers shortest parent
	};

	const int nMapWidth = WINDOW_SIZE.x / 32;
	const int nMapHeight = WINDOW_SIZE.y / 32;

	const int nNodeSize = 32;
	const int nNodeBorder = 9;
	sNode* nodes = nullptr;

	sNode* nodeStart = nullptr;
	sNode* nodeEnd = nullptr;

	sf::Vector2i mousePos;


	sf::RenderWindow* window = nullptr;

	bool b8Connection = false;

	sf::RectangleShape shape;
};

int main() {
	sf::RenderWindow window{sf::VideoMode{WINDOW_SIZE.x,WINDOW_SIZE.y},"SFML Sandbox"};
	window.setFramerateLimit(10);
	window.setPosition(sf::Vector2i{window.getPosition().x,0});
	sf::Clock dtClock;


	WavePropagation wave;
	AStar astar(&window);

	bool isStar = false;

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed || event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				switch (event.key.code) {
					case sf::Keyboard::Enter: cout << "Enter Pressed\n"; break;
					case sf::Keyboard::Space: cout << "Space Pressed\n"; break;
					default: break;
				}
			}
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) {
			isStar = !isStar;
		}

		window.clear();

		if (isStar)
			astar.onUpdate(dtClock.restart().asSeconds(), sf::Mouse::getPosition(window));
		else
			wave.onUserUpdate(&window,dtClock.restart().asSeconds());


		window.display();

	}
	return 0;
}