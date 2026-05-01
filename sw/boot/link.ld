OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
  /* O SuperNova-RV tem uma memória unificada começando no 0x0 */
  ram (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
}

SECTIONS
{
  /* Seção de código (Text) */
  .text :
  {
    *(.text.init) /* Garante que o startup code venha primeiro */
    *(.text)
    *(.text.*)
  } > ram

  /* Seção de dados globais */
  .data :
  {
    *(.data)
    *(.data.*)
    *(.sdata)
    *(.sdata.*)
  } > ram

  /* Seção de dados não inicializados */
  .bss :
  {
    *(.bss)
    *(.bss.*)
    *(.sbss)
    *(.sbss.*)
  } > ram

  _end = .;
}