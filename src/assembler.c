
#include "assembler.h"

/**
* Tokenizer
*/

void error( char* msg, LexState state )
{
  printf("Error: %s @ line %d col %d \n", msg, state->line, state->column);
  exit(1);
}

int read( LexState state )
{
  int c = fgetc( state->in );
  if( c == '\n' )
  {
    state->line++;
    state->column = 1;
  }
  else
  {
    state->column++;
  }
  return c;
}

void unread( int c, LexState state )
{
  if( c == '\n' )
  {
    state->line--;
    state->column = 0;
  }
  else
  {
    state->column--;
  }
  ungetc( c, state->in );
}

int peek( LexState state )
{
  int c = read( state );
  unread( c, state );
  return c;
}

void expect( int expected, LexState state )
{
  int c = read( state );
  if( c != expected )
  {
    printf( "Expected '%c', found '%c' @ line %d col %d", 
            expected, c, state->line, state->column );
    error( "", state );
  }
  return;
}

void skip_ws( LexState state )
{
  int c = -1;
  do
  {
    c = read( state );
    if( c == ';' )
    {
      // ignore the rest of the line
      while( c != -1 && c != '\n' )
      {
        c = read( state );
      }
    }
  }  
  while( isspace(c) );
  
  unread( c, state );
}

bool is_symbol_char( int c )
{
  return c == '.' || isalpha( c );
}

void read_symbol( LexState state )
{
  // collect
  state->buf_len = 0;
  
  int c = read( state );
  while( isalpha( c ) )
  {
    c = tolower( c );

    if( state->buf_len >= BUF_SIZE )
    {
      error( "Maximum symbol length exceeded", state );
    }
    state->buf[state->buf_len] = c;
    state->buf_len++;
  
    c = read( state );
  }
  unread( c, state );
  
  // terminate string
  state->buf[state->buf_len] = 0;
}

Token read_token( LexState state )
{
  skip_ws( state );
  
  int c = read( state );
  if( c == -1 )
  {
    return TOKEN_EOF;
  }
  
  if( isdigit( c ) )
  {
    if( c == '0' )
    {
      return (Token){ TT_CONST, CONST_0 };
    }
    else if( c == '1' )
    {
      return (Token){ TT_CONST, CONST_1 };
    }
    else
    {
      error( "Only constant values zero and one are allowed", state );
      return TOKEN_EOF;
    }
  }
  else if( c == 'x' )
  {
    int peek_char = peek( state );
    if( isdigit( peek_char ) 
        || (peek_char >= 'a' && peek_char <= 'f')
        || (peek_char >= 'A' && peek_char <= 'F') )
    {
      uint16_t address = read_hex( state );
      
      return (Token){ TT_ADDR, address };
    }
    unread( c, state );
  }
  else if( c == '[' )
  {
    Token inner = read_token( state );
    expect( ']', state );
    
    if( inner.type == TT_REG )
    {
      return (Token){ TT_REG_MEM, inner.value };
    }
    else
    {
      error( "Expected register", state );
    }
  }
  else if( c == 'r' )
  {
    int next_char = read( state );
    if( next_char >= '0' && next_char <= '9' )
    {
      // ASCII offset from digit characters to digit values
      uint16_t value = next_char - 48;
      if( value > 7 )
      {
        error( "Expected register number", state );
      }
      return (Token){ TT_REG, value };
    }
    else
    {
      unread( next_char, state );
    }
  }
  else if( c == '.' )
  {
    // directive
    read_symbol( state );
    
    if( strcmp( "org", state->buf ) == 0 )
    {
      return (Token){ TT_DIR, DR_ORG };
    }
  }
  
  if( isalpha( c ) )
  {
    // unread the char so the symbol reader can get to it
    unread( c, state );
    
    read_symbol( state );
    
    if( state->buf_len == 1 )
    {
      int val = state->buf[0];
      switch( val )
      {
        case 'a': return (Token){ TT_CONST, CONST_A };
        case 'b': return (Token){ TT_CONST, CONST_B };
        case 'c': return (Token){ TT_CONST, CONST_C };
      }
    }
    
    // see if the symbol collected is a mnemonic
    int i;
    for( i=0; i<MNEMONIC_COUNT; i++ )
    {
      if( strcmp( MNEMONICS[i], state->buf ) == 0 )
      {
        // i will be the ordinal of the 
        return (Token){ TT_INSTR, i };
      }
    }
    
    if( state->buf_len >= 3 )
    {
      uint8_t flags = 0;
      if( state->buf[0] == 'j'
        && state->buf[1] == 'm'
        && state->buf[2] == 'p' )
      {
        int i = 3;
        if( i < state->buf_len && state->buf[i] == 'p' )
        {
          flags |= COND_P;
          i++;
        }
        
        if( i < state->buf_len && state->buf[i] == 'z' )
        {
          flags |= COND_Z;
          i++;
        }
        
        if( i < state->buf_len && state->buf[i] == 'n' )
        {
          flags |= COND_N;
          i++;
        }
        return (Token){ TT_INSTR, MN_JMP, flags };
      }
    }
    
    // if not, assume it's a label
    skip_ws( state );
    int next_char = read( state );
    if( next_char == ':' )
    {
      // label
      return (Token){ TT_LABEL_DEF, 0 };
    }
    else
    {
      // unread the peek colon char
      unread( next_char, state );
      
      return (Token){ TT_LABEL, 0, };
    }
    
    error( "Expected mnemonic or label", state );
  }
  
  error( "Bad token", state );
  
  return TOKEN_EOF;
}

uint16_t read_hex( LexState state )
{
  int c = read( state );
  
  if( isalnum(c) )
  {
    int char_count = 0;
    uint16_t value = 0;
    
    do
    {
      if( char_count > 3 )
      {
        error( "Overflow of unsigned 16-bit integer", state );
      }
      
      value = value << 4;

      switch( c )
      {
        case '0': break;
        case '1': value += 1; break;
        case '2': value += 2; break;
        case '3': value += 3; break;
        case '4': value += 4; break;
        case '5': value += 5; break;
        case '6': value += 6; break;
        case '7': value += 7; break;
        case '8': value += 8; break;
        case '9': value += 9; break;
        case 'a':
        case 'A': value += 10; break;
        case 'b': 
        case 'B': value += 11; break;
        case 'c': 
        case 'C': value += 12; break;
        case 'd': 
        case 'D': value += 13; break;
        case 'e': 
        case 'E': value += 14; break;
        case 'f': 
        case 'F': value += 15; break;
        default:
          error( "Expected hex char", state );
      }
      char_count++;
      c = read( state );
    }
    while( isalnum(c) );
    
    unread( c, state );
    return value;
  }

  error( "Expected hex char", state );
  return 0;
}

void add_label( char* label_name, uint8_t pos, LexState state )
{
  //printf("label: %s \n", label_name);
  Label* label = malloc( sizeof(Label) );
  strncpy( label->label, label_name, strlen(label_name) );
  label->pos = pos;
  
  // make the new label the root of the state.labels linked list
  label->next = state->labels;
  state->labels = label;
}

/**
* Returns either the address associated with the given label,
* or -1 if the label was not found
*/
int find_label( char* label_name, LexState state )
{
  Label* label = state->labels;
  while( label != NULL )
  {
    if( strcmp( label->label, label_name ) == 0 )
    {
      return label->pos;
    }
    
    label = label->next;
  }
  return -1;
}

/**
* Add the given instruction to the list of label fix ups
*/
void fixup_label( char* label, uint8_t minstr_offset, LexState state )
{
  LabelFixup* fixup = malloc( sizeof(Label) );
  strncpy( fixup->label, label, strlen(label) );
  fixup->instr_offset = minstr_offset;
  
  // make the new label the root of the state.labels linked list
  fixup->next = state->fixups;
  state->fixups = fixup;
}





/**
* Returns number of trailing zeros
*
* Hackers Delight, Chapter 5, p. 85
*/

int ntz(uint32_t x)
{
  int n;
  
  if( x == 0 ) return 32;
  n = 1;
  if((x & 0x0000FFFF) == 0) {n = n +16; x = x >>16; }
  if((x & 0x000000FF) == 0) {n = n + 8; x = x >> 8; }
  if((x & 0x0000000F) == 0) {n = n + 4; x = x >> 4; }
  if((x & 0x00000003) == 0) {n = n + 2; x = x >> 2; }
  return n - (x & 1);
}

void minstr_set( MicroInstruction* minstr, uint32_t field, uint32_t value )
{
  // clear the existing bits of the field
  *minstr &= ~field;
  
  // or the value into minstr, shifted by the number
  // of leading zeros of the field
  *minstr |= value << ntz(field);
}

uint32_t minstr_get( MicroInstruction minstr, uint32_t field )
{
  return (minstr & field) >> ntz(field);
}

void write_minstr( MicroInstruction minstr, LexState state )
{
  printf("write: 0x%X \n", minstr);
  if( minstr_get( minstr, M_MODE ) == 0 )
  {
    printf(" mw:%X aa:%X mb:%X ba:%X mf:%X fs:%X da:%X rw:%X \n",
            minstr_get( minstr, M_MW ),
            minstr_get( minstr, M_AA ),
            minstr_get( minstr, M_MB ),
            minstr_get( minstr, M_BA ),
            minstr_get( minstr, M_MF ),
            minstr_get( minstr, M_FS ),
            minstr_get( minstr, M_DA ),
            minstr_get( minstr, M_RW ) );
    
    
  }
  else
  {
    printf(" cond:%X next_addr:%X  \n",
            minstr_get( minstr, M_COND ),
            minstr_get( minstr, M_NEXT_ADDR ) );
  }
  
  if( state->instr_pos == ROM_SIZE - 1 )
  {
    error( "ROM storage exceeded", state );
    return;
  }
  
  state->instructions[state->instr_pos] = minstr;
  state->instr_pos++;
}


void fixup_labels( LexState state )
{
  LabelFixup* fixup = state->fixups;
  while( fixup != NULL )
  {
    int offset = find_label( fixup->label, state );
    if( offset == -1 )
    {
      error( "Unknown label", state );
      return;
    }
    MicroInstruction* minstr = &state->instructions[fixup->instr_offset];
    minstr_set( minstr, M_NEXT_ADDR, offset );
    
    fixup = fixup->next;
  }
}


void parse_instruction( LexState state, Token instr )
{
  MicroInstruction minstr;
  memset( &minstr, 0, sizeof(MicroInstruction) );
  
  switch( instr.value )
  {
    case MN_NOP: break;
    
    case MN_MOV:
    {
      Token dst = read_token( state );
      Token src = read_token( state );
      if( dst.type == TT_REG )
      {
        // destination is a register, so we need to enable regfile write flag
        minstr_set( &minstr, M_RW, 1 );
        
        // destination register will be the 'dst' value
        minstr_set( &minstr, M_DA, dst.value );
        
        // B bus is used exclusively for values (A for addresses)
        minstr_set( &minstr, M_FS, F_B );
        
        // populate B bus with the value of the src register or address
        minstr_set( &minstr, M_BA, src.value );
        
        // moving to register
        // we can move from another register, memory, or a constant
        //
        // use B Address of data path for all src values
        if( src.type == TT_REG )
        {
            // mov rX rY
        }
        else if( src.type == TT_REG_MEM )
        {
          // mov rX [rY]
          //
          // we're pulling a value from memory, so set muxF to receive
          // the response from the memory unit
          minstr_set( &minstr, M_MF, 1 );
          
          // memory access uses the A bus, which needs the register number
          minstr_set( &minstr, M_AA, src.value );
        }
        else if( src.type == TT_CONST )
        {
          // mov rX 7
          //
          // set muxB to put constIn on B bus
          if( src.value == CONST_1 )
          {
            // the only way to deliver a constant one is through
            // the function unit, 'F=1'
            minstr_set( &minstr, M_FS, F_1 );
          }
          else
          {
            // all other constants (A,B,C,0) are delivered through constIn
            minstr_set( &minstr, M_MB, 1 );
          }
        }
        else
        {
            error( "Can only move another register, memory, or a constant to a register", state );
        }
      }
      else if( dst.type == TT_REG_MEM )
      {
        // regardless of where it is coming from,
        // the A Address must specify where it is going to be written
        minstr_set( &minstr, M_AA, dst.value );
        
        // we're writing to memory, so enable memory write
        minstr_set( &minstr, M_MW, 1 );
        
        if( src.type == TT_REG )
        {
          // mov [rX] rY
          
          minstr_set( &minstr, M_BA, src.value );
        }
        else if( src.type == TT_CONST )
        {
          // mov [rX] 0|A|B|C
          if( src.value == CONST_1 )
          {
            error( "Can only move const 0 directly to memory", state );
          }
          
          // (M_BA aliases for CONST codes)
          minstr_set( &minstr, M_BA, src.value );
          
          // switch in the constant value
          minstr_set( &minstr, M_MB, 1 );
        }
        else
        {
          error( "Can only move a register to memory", state );
        }
      }
      else
      {
        error( "mov destination must be register or memory", state );
      }
      break;
    }
    
    // three-register operand instructions
    case MN_ADD:
    case MN_SUB:
    case MN_MUL:
    case MN_DIV:
    case MN_AND:
    case MN_OR:
    {
      Token dst = read_token( state );
      Token left = read_token( state );
      Token right = read_token( state );
      
      if( dst.type != TT_REG || left.type != TT_REG || right.type != TT_REG )
      {
        error( "operands must be registers", state );
      }
      
      minstr_set( &minstr, M_RW, 1 );
      minstr_set( &minstr, M_DA, dst.value );
      minstr_set( &minstr, M_AA, left.value );
      minstr_set( &minstr, M_BA, right.value );
      
      switch( instr.value )
      {
        case MN_ADD: minstr_set( &minstr, M_FS, F_ADD ); break;
        case MN_SUB: minstr_set( &minstr, M_FS, F_SUB ); break;
        case MN_MUL: minstr_set( &minstr, M_FS, F_MUL ); break;
        case MN_DIV: minstr_set( &minstr, M_FS, F_DIV ); break;
        case MN_AND: minstr_set( &minstr, M_FS, F_AND ); break;
        case MN_OR:  minstr_set( &minstr, M_FS, F_OR ); break;
      }
      break;
      
    }
    
    // two-register operand instructions
    case MN_NOT:
    case MN_NADD:
    case MN_RSH:
    case MN_LSH:
    case MN_SAR:
    {
      Token dst = read_token( state );
      Token arg = read_token( state );
      
      if( dst.type != TT_REG || arg.type != TT_REG )
      {
        error( "operands must be registers", state );
      }
      
      minstr_set( &minstr, M_RW, 1 );
      minstr_set( &minstr, M_DA, dst.value );
      minstr_set( &minstr, M_AA, arg.value );
      
      switch( instr.value )
      {
        case MN_NOT: minstr_set( &minstr, M_FS, F_NOT ); break;
        case MN_NADD: minstr_set( &minstr, M_FS, F_NADD ); break;
        case MN_RSH: minstr_set( &minstr, M_FS, F_RSH ); break;
        case MN_LSH: minstr_set( &minstr, M_FS, F_LSH ); break;
        case MN_SAR: minstr_set( &minstr, M_FS, F_SAR ); break;
      }
    
      break;
    }
    
    case MN_JMP:
    {
      Token addr = read_token( state );
      if( addr.type == TT_ADDR || addr.type == TT_LABEL )
      {
        minstr_set( &minstr, M_MODE, 1 );
        minstr_set( &minstr, M_COND, instr.flags );
        
        if( addr.type == TT_LABEL )
        {
          int label_offset = find_label( state->buf, state );
          if( label_offset == -1 )
          {
            // we are referencing a label that either doesn't exist
            // or hasn't been defined yet, so add it as a fixup
            fixup_label( state->buf, state->instr_pos, state );
          }
          else
          {
            // the label has been defined, so set its associated address
            minstr_set( &minstr, M_NEXT_ADDR, label_offset );
          }
        }
        else if( addr.type == TT_ADDR )
        {
          minstr_set( &minstr, M_NEXT_ADDR, addr.value );
        }
      }
      else
      {
        error( "Expected address or label for jump instruction", state );
      }
    
      break;
    }
    
    default:
      error( "Unimplemented instruction", state );
  }
  
  write_minstr( minstr, state );
}

void parse_directive( LexState state, Token dir )
{
  switch( dir.value )
  {
    case DR_ORG:
    {
      Token addr = read_token( state );
      if( addr.type != TT_ADDR )
      {
        printf( "%d", addr.type );
        error( "Expected address", state );
      }
      
      // reposition offset into instruction stream to the requested position
      // 
      state->instr_pos = addr.value;
      
      break;
    }
    
    default:
    error( "Expected directive", state );
  }
  return;
}



void parse_microcode( LexState state )
{
  bool labelled = false;
  
  Token token = read_token( state );
  while( token.type != TT_EOF )
  {
    if( labelled )
    {
      if( token.type != TT_INSTR )
      {
        error( "Instruction must follow label", state );
        return;
      }
      labelled = false;
    }
    
    if( token.type == TT_INSTR )
    {
      parse_instruction( state, token );
    }
    else if( token.type == TT_DIR )
    {
      parse_directive( state, token );
    }
    else if( token.type == TT_LABEL_DEF )
    {
      if( find_label( state->buf, state ) == -1 )
      {
        // label hasn't been used before, create it
        add_label( state->buf, state->instr_pos, state );
        labelled = true;
      }
      else
      {
        error( "Label already defined", state );
        return;
      }
    }

    token = read_token( state );
  }
}

bool is_big_endian(void)
{
  union {
      uint32_t i;
      char c[4];
  } bint = {0x01020304};

  return bint.c[0] == 1; 
}

void write_binary( FILE* out_file, LexState state )
{
  if( is_big_endian() )
  {
    MicroInstruction* instr_ptr = state->instructions;
    fwrite( instr_ptr, 3, ROM_SIZE, out_file );
  }
  else
  {
    uint8_t* instr_ptr = (uint8_t*)state->instructions;
    int i;
    for( i=0; i < ROM_SIZE; i++ )
    {
      if( i > 0 )
      {
        fprintf( out_file, " " );
      }
      instr_ptr += 2; // skip to last byte of instruction
      fwrite( instr_ptr, 1, 1, out_file );
      instr_ptr--;
      fwrite( instr_ptr, 1, 1, out_file );
      instr_ptr--;
      fwrite( instr_ptr, 1, 1, out_file );
      instr_ptr += 4; // skip to next instruction
    }
  }
}

void write_logisim_byte( FILE* out_file, uint8_t byte )
{
  if( byte < 16 )
  {
    fprintf( out_file, "0%X", byte );
  }
  else
  {
    fprintf( out_file, "%X", byte );
  }
}

/**
* Write the output in logisim format.  From the Logisim Documentation:

  The file format used for image files is quite simple; the intention is that a 
  user can write a program, such as an assembler, that generates memory images 
  that can then be loaded into the RAM. As an example of this file format, if 
  we had a 256-byte memory whose first five bytes were 2, 3, 0, 20, and -1, and 
  all subsequent values were 0, then the image would be the following text file.

  v2.0 raw
  02
  03
  00
  14
  ff
  
  The first line identifies the file format used (currently, there is only one 
  file format recognized). Subsequent lines list the values in little-endian 
  hexadecimal. Logisim will assume that any values unlisted in the file are 
  zero.
*/
void write_logisim( FILE* out_file, LexState state )
{
  fprintf( out_file, "v2.0 raw\x0A" );
  
  if( is_big_endian() )
  {
    uint8_t* instr_ptr = (uint8_t*)state->instructions;
    int i;
    for( i=0; i < ROM_SIZE; i++ )
    {
      if( i > 0 )
      {
        fprintf( out_file, " " );
      }
      write_logisim_byte( out_file, *instr_ptr );
      instr_ptr++;
      write_logisim_byte( out_file, *instr_ptr );
      instr_ptr++;
      write_logisim_byte( out_file, *instr_ptr );
      instr_ptr+=2;
    }
  }
  else
  {
    uint8_t* instr_ptr = (uint8_t*)state->instructions;
    int i;
    for( i=0; i < ROM_SIZE; i++ )
    {
      if( i > 0 )
      {
        fprintf( out_file, " " );
      }
      instr_ptr += 2; // skip to last byte of instruction
      write_logisim_byte( out_file, *instr_ptr );
      instr_ptr--;
      write_logisim_byte( out_file, *instr_ptr );
      instr_ptr--;
      write_logisim_byte( out_file, *instr_ptr );
      instr_ptr += 4; // skip to next instruction
    }
    
  }
}

int main( int argc, const char* argv[] )
{
  FILE* src_file = NULL;
  FILE* out_file = NULL;
  bool binary_output = false;
  int arg_pos = 1;
  
  if( arg_pos < argc )
  {
    if( strcmp( "-r", argv[arg_pos] ) == 0 )
    {
      binary_output = true;
      arg_pos++;
    }
  }
  
  if( arg_pos < argc )
  {
    src_file = fopen(argv[arg_pos], "r");
    arg_pos++;
  }
  
  if( arg_pos < argc )
  {
    out_file = fopen( argv[arg_pos], "wb" );
  }
  
  if( out_file == NULL )
  {
    out_file =  stdout;
  }
  
  if( src_file == NULL )
  {
    printf("Expected dda srcfile [outfile]\n");
    return 1;
  }
  
  LexState state = &(struct LexState){ src_file, 1, 1, 0, 0 };
  memset( &state->instructions, 0, ROM_SIZE );
  
  // parse the microcode, collecting the instruction stream in state
  parse_microcode( state );
  
  fixup_labels( state );
  
  // write the instructions (in the correct byte order)
  // to the file
  if( binary_output )
  {
    write_binary( out_file, state );
  }
  else
  {
    write_logisim( out_file, state );
  }
  
  fclose( out_file );
  
  fclose( src_file );
  

  return 0;
}