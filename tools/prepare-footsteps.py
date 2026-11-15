"""Rebuild current player/monster banks from documented CC0 sources.

The older supplied-MP3 extraction workflow is retired. This script cannot
import arbitrary replacement recordings into the distributable bank.
"""
from prepare_release_audio import prepare_footsteps

if __name__ == '__main__':
    prepare_footsteps()
