
### Tile Powers

- how do these resolve on the board state since the board state is server side?
- Take Magnet tile power (if a magnet tile is apart of a scoring combination, the nearest pieces in each cardinal direction get pulled towards the tile by n)


### Tile Power Board Change Flow

- Create queue copying all tiles effected by board operation (this should be a struct that holds the tile and what happened to it (piece placed, piece removed, shifted, etc.))
- While loop, pop first in queue, check if tile has power.
	- if post scoring and matches effect type (piece enter/exit) of board operation then add to post scoring array
	- if pre scoring, then pass board to tile power operation (alters board and returns tiles which were touched and why). Add these returned types to end of queue