<div align="center">

# Cataphract
___
</div>

A UCI chess engine written in C++.

## Features:
### Search:
* Principal variation search with iterative deepening
* Aspiration windows
* Futility pruning and reverse futility pruning
* Razoring
* Mate distance pruning
* Null move pruning
* Late move reductions
* Late move pruning
* Improving heuristic
* History pruning
* Internal iterative reduction
* Killers (2 per ply)
* Static exchange evaluation
* Probcut
#### History:
* Main history (Piece-to and butterfly)
* Countermove history (1-ply), follow-up history (2-ply), and 4-ply history.
* Capture history
#### Correction history:
* Pawn correction history
* Minor piece correction history
* Major piece correction history
### Evaluation
#### NNUE 
* Architecture: (768x10 -> 1536)x2 -> 1x8, horizontally mirrored.
* Trained using Lc0's data using the [Bullet](https://github.com/jw1912/bullet) trainer developed by Jamie Whiting.

> Only work with CPUs that support AVX2.
