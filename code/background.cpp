



DWORD WINAPI RunBackgroundLoadingThread( LPVOID lpParam )
{
    while (running)
    {

        for (int i = 0; i < tiles.count; i++) {
            if (tiles[i].needs_loading) {
                if (!tiles[i].is_media_loaded) {
                    tiles[i].LoadMedia();
                }
            }
        }

        Sleep(16); // hmm

    }
    return 0;
}

void LaunchBackgroundLoadingLoop() {
    CreateThread(0, 0, RunBackgroundLoadingThread, 0, 0, 0);
}



DWORD WINAPI RunBackgroundUnloadingThread( LPVOID lpParam )
{
    while (running)
    {

        for (int i = 0; i < tiles.count; i++) {
            if (tiles[i].needs_unloading) {
                if (tiles[i].is_media_loaded) {
                    tiles[i].UnloadMedia();
                }
            }
        }

        Sleep(16); // hmm

    }
    return 0;
}

void LaunchBackgroundUnloadingLoop() {
    CreateThread(0, 0, RunBackgroundUnloadingThread, 0, 0, 0);
}

