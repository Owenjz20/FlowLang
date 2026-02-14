using System;
using System.Collections.Generic;

namespace FlowLangRuntime
{
    public class FlowGameEngine
    {
        private readonly Action<string> _write;

        public FlowGameEngine(Action<string> write)
        {
            _write = write;
        }

        public void RunGame(List<string> lines)
        {
            _write("Game engine not fully implemented yet, but hook is ready.");
            // You can parse game "name":, scene, player, enemy, on update, etc.
            // and run a game loop here.
        }
    }
}
