#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>

enum class DemoFrameType : uint8_t {
	NET_MSG_DATA = 0,
	NET_MSG = 1,
	DEMO_START = 2,
	NET_CHAN = 3,
	UNKNOWN = 4,
	CONSOLE_COMMAND = 5,
	CLIENT_DATA = 6,
	DEMO_BUFFER = 7,
	NEXT_SECTION = 8
	// SOUND = 8,
	// DEMO_BUFFER = 9
};

struct Origin {
	float x, y, z;
};

struct ViewanglesOrigin {
	float pitch, x, yaw, y, roll, z;
};

int main(int argc, char *argv[])
{
	if (argc != 2) {
		std::cout << "won-demo-parser.exe <demoname>\n";
		return 1;
	}

	std::ifstream input(argv[1], std::ios::binary);

	if (!input.is_open()) {
		std::cout << "couldn't open the file.\n";
		return 1;
	}

	char szFileStamp[6];
	input.read(szFileStamp, sizeof(szFileStamp));
	if (std::strncmp(szFileStamp, "HLDEMO", 6)) {
		std::cout << "not a goldsrc demo.\n";
		return 1;
	}

	int nNetProtocol = 0;
	input.seekg(12, std::ios_base::beg);
	input.read(reinterpret_cast<char*>(&nNetProtocol), sizeof(int32_t));

	if (nNetProtocol != 40) {
		std::cout << "unsupported net protocol: " << nNetProtocol << std::endl;
		return 1;
	}
	
	int nDirectoryOffset = 0;
	input.seekg(540, std::ios_base::beg);
	input.read(reinterpret_cast<char*>(&nDirectoryOffset), sizeof(int32_t));

	input.seekg(nDirectoryOffset, std::ios_base::beg);

	int nEntries = 0;
	input.read(reinterpret_cast<char*>(&nEntries), sizeof(int32_t));
	std::cout << "parsing the demo file...\n";

	for (int i = 0; i < nEntries; ++i) {
		int nFrames = 0, nOffset = 0;
		input.seekg(80, std::ios_base::cur);
		input.read(reinterpret_cast<char*>(&nFrames), sizeof(int32_t));
		input.read(reinterpret_cast<char*>(&nOffset), sizeof(int32_t));

		input.seekg(nOffset, std::ios_base::beg);

		DemoFrameType cmd{};
		int bufferSize = 0, msglen = 0;
		char ccBuffer[64];
		Origin origin{};  ViewanglesOrigin viewanglesOrigin{};
		//lets count the frames
		static int ds, cc, cd, db, nc, unk, nmd, nm;

		do {
			input.read(reinterpret_cast<char*>(&cmd), sizeof(uint8_t));
			input.seekg(8, std::ios_base::cur);

			switch (cmd) {
			case DemoFrameType::DEMO_START:
				ds++;
				break;
			case DemoFrameType::CONSOLE_COMMAND:
				// see whats inside
				input.read(ccBuffer, sizeof(ccBuffer));
				// safe to put here, because there aren't many of them
				std::cout << "ccBuffer: " << ccBuffer << std::endl;
				cc++;
				break;
			case DemoFrameType::CLIENT_DATA:
				// old clientdata_t
				//input.seekg(64, std::ios_base::cur);
				input.read(reinterpret_cast<char*>(&origin.x), sizeof(float));
				input.read(reinterpret_cast<char*>(&origin.y), sizeof(float));
				input.read(reinterpret_cast<char*>(&origin.z), sizeof(float));
				// there's more data here like fov and stuff
				input.seekg(52, std::ios_base::cur);
				cd++;
				break;
			case DemoFrameType::NEXT_SECTION:
				break;
			case DemoFrameType::DEMO_BUFFER:
				input.read(reinterpret_cast<char*>(&bufferSize), sizeof(int32_t));
				input.seekg(bufferSize, std::ios_base::cur);
				db++;
				break;
			case DemoFrameType::NET_CHAN:
				input.seekg(28, std::ios_base::cur);
				nc++;
				break;
			case DemoFrameType::UNKNOWN:
				unk++;
				break;
			case DemoFrameType::NET_MSG_DATA:
				//pairs of viewangles+origin
				input.seekg(24, std::ios_base::cur);
				input.read(reinterpret_cast<char*>(&msglen), sizeof(int32_t));
				input.seekg(msglen, std::ios_base::cur);
				nmd++;
				break;
			case DemoFrameType::NET_MSG:
				//pairs of viewangles+origin
				//input.seekg(24, std::ios_base::cur);
				input.read(reinterpret_cast<char*>(&viewanglesOrigin.pitch), sizeof(float));
				input.read(reinterpret_cast<char*>(&viewanglesOrigin.x), sizeof(float));
				input.read(reinterpret_cast<char*>(&viewanglesOrigin.yaw), sizeof(float));
				input.read(reinterpret_cast<char*>(&viewanglesOrigin.y), sizeof(float));
				input.read(reinterpret_cast<char*>(&viewanglesOrigin.roll), sizeof(float));
				input.read(reinterpret_cast<char*>(&viewanglesOrigin.z), sizeof(float));
				input.read(reinterpret_cast<char*>(&msglen), sizeof(int32_t));
				input.seekg(msglen, std::ios_base::cur);
				nm++;
				break;
			}
		} while (cmd != DemoFrameType::NEXT_SECTION);

		if (i == 0) {
			input.seekg(nDirectoryOffset, std::ios_base::beg);
			input.seekg(4, std::ios_base::cur);
			input.seekg(92, std::ios_base::cur);
		} else {
			std::cout << "reached the end of the demo\n";
			std::cout << "Last origin: " << origin.x << " " << origin.y << " " << origin.z << '\n';
			std::cout << "Last whatever the heck this is: "
				<< viewanglesOrigin.pitch << " "
				<< viewanglesOrigin.x     << " "
				<< viewanglesOrigin.yaw   << " "
				<< viewanglesOrigin.y     << " "
				<< viewanglesOrigin.roll  << " "
				<< viewanglesOrigin.z     << " " << std::endl;
			float time = 0.0f; int frame = 0;
			input.seekg(-8, std::ios_base::cur);
			input.read(reinterpret_cast<char*>(&time), sizeof(float));
			input.read(reinterpret_cast<char*>(&frame), sizeof(int32_t));
			std::cout << "Recording time:\t" << std::fixed << std::setprecision(2) << time << '\n';
			std::cout << "Host frames:\t" << frame << '\n';
			std::cout << '\n';

			std::cout << "NET_MSG_DATA:\t" << nmd << '\n';
			std::cout << "NET_MSG:\t" << nm << '\n';
			std::cout << "DEMO_START:\t" << ds << '\n';
			std::cout << "NET_CHAN:\t" << nc << '\n';
			std::cout << "UNKNOWN:\t" << unk << '\n';
			std::cout << "CONSOLE_COMMAND:" << cc << '\n';
			std::cout << "CLIENT_DATA:\t" << cd << '\n';
			std::cout << "DEMO_BUFFER:\t" << db << '\n';
			break;
		}
	}
	return 0;
}
